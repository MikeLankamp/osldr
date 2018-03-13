#include <bios.h>
#include <string.h>
#include <drive.h>

#define DRIVE_INFO_LENGTH    16    /* Wise to make it a power of 2 */

/* BIOS Disk base table (for Int 13h) */
typedef struct _DISK_BASE_TABLE
{
    UCHAR Specify1;
    UCHAR Specify2;
    UCHAR DiskMotorShutoffDelay;
    UCHAR BytesPerSector;
    UCHAR SectorsPerTrack;
    UCHAR InterBlockGapLength;
    UCHAR DataLength;
    UCHAR GapLength;
    UCHAR FillByte;
    UCHAR HeadSettleTime;
    UCHAR MotorStartupTime;
} PACKED DISK_BASE_TABLE;

static USHORT     DriveParametersCount = 0;
static DRIVE_INFO DriveParameters[ DRIVE_INFO_LENGTH ];

BOOL ResetDrive( UCHAR Drive )
{
    REGS regs;

    regs.h.ah = 0x00;
    regs.h.dl = Drive;

    int86( 0x13, &regs, &regs );

    return (regs.x.cflag == 0);
}

DRIVE_INFO* GetDriveParameters( UCHAR Drive )
{
    DRIVE_INFO* pdi;
    REGS        regs;
    UINT        i;

    /* First check if information was retrieved earlier */
    for (i = 0; i < DriveParametersCount; i++)
    {
        if (Drive == DriveParameters[i].Drive)
        {
            /* Yes, return that one.
             * By moving it to the front we create an LRU administration.
             */
            DRIVE_INFO di = DriveParameters[i];
            memmove( &DriveParameters[1], &DriveParameters[0], i * sizeof(DRIVE_INFO) );
            DriveParameters[0] = di;
            return &DriveParameters[0];
        }
    }

    /* Not cached, read */
    if (DriveParametersCount == DRIVE_INFO_LENGTH)
    {
        /* Static buffer is full, throw out LRU info */
        DriveParametersCount--;
    }

    /* We add the new info at the beginning */
    memmove( &DriveParameters[1], &DriveParameters[0], DriveParametersCount++ * sizeof(DRIVE_INFO) );
    pdi = &DriveParameters[0];

    pdi->Drive = Drive;

    /* Check for INT13h Extensions */
    regs.h.ah = 0x41;
    regs.x.bx = 0x55aa;
    regs.h.dl = Drive;
    int86( 0x13, &regs, &regs );

    if ((!regs.x.cflag) && (regs.x.bx == 0xaa55))
    {
        /* INT 13h extensions supported */
        pdi->EddVersion      = regs.h.ah;
        pdi->ControllerFlags = regs.x.cx;
        if (regs.x.cx & 1)
        {
            /* ControllerFlags temporarily acts as var for INT 13h/AH=48h */
            USHORT Flags = pdi->ControllerFlags;

            pdi->ControllerFlags = 0x1A; /* We just want v1.x info */
            regs.h.ah = 0x48;
            regs.h.dl = Drive;
            regs.x.ds = SEG( &pdi->ControllerFlags );
            regs.x.si = OFS( &pdi->ControllerFlags );
            int86( 0x13, &regs, &regs );

            /* Restore flags */
            pdi->ControllerFlags = Flags;

            if (!regs.x.cflag)
            {
                return pdi;
            }
        }
    }

    /* INT 13h extensions NOT supported or no INT13h/AH=48h */
    pdi->EddVersion      = 0;
    pdi->ControllerFlags = 0;
    pdi->DriveFlags      = 0;
    pdi->nBytesPerSector = 512;

    regs.h.ah = 8;
    regs.h.dl = Drive;
    regs.x.es = 0;
    regs.x.di = 0;
    int86( 0x13, &regs, &regs );
    if (!regs.x.cflag)
    {
        /* Success */
        DISK_BASE_TABLE* dbt = PTR( regs.x.es, regs.x.di );

        pdi->nHeads        = (ULONG)regs.h.dh + 1;
        pdi->nSectors      = (ULONG)regs.h.cl & 0x3F;
        pdi->nCylinders    = (((ULONG)(regs.h.cl & 0xC0) << 2) | regs.h.ch) + 1;
        pdi->nTotalSectors = pdi->nHeads * pdi->nSectors * pdi->nCylinders;

        if (dbt->BytesPerSector < 4)
        {
            /* Use BytesPerSector from BIOS Disk base table if valid */
            pdi->nBytesPerSector = 128 << dbt->BytesPerSector;
        }

        return pdi;
    }

    /* Failure */
    DriveParametersCount--;
    return NULL;
}

ULONGLONG ReadDrive( UCHAR Drive, ULONGLONG Sector, ULONGLONG nSectors, VOID* Buffer )
{
    ULONG       Read = 0;
    REGS        regs;
    DRIVE_INFO* pdi  = GetDriveParameters( Drive );

    if (pdi == NULL)
    {
        return 0;
    }

    if (pdi->ControllerFlags & 1)
    {
        /* EDD Supported */
        struct DISK_ADDRESS_PACKET
        {
            UCHAR     Size;
            UCHAR     Reserved;
            USHORT    nBlocks;
            ULONG     Buffer;
            ULONGLONG StartBlock;
        } PACKED addrpack;

        addrpack.Size       = 16;
        addrpack.Reserved   = 0;
        addrpack.nBlocks    = nSectors;
        addrpack.Buffer     = MAKELONG( SEG(Buffer), OFS(Buffer) );
        addrpack.StartBlock = Sector;

        regs.h.ah = 0x42;
        regs.h.dl = Drive;
        regs.x.ds = SEG( &addrpack );
        regs.x.si = OFS( &addrpack );
        int86( 0x13, &regs, &regs );

        if ((regs.x.cflag) || (regs.h.ah != 0))
        {
            return 0;
        }

        return addrpack.nBlocks;
    }

    /* EDD NOT Supported */
    while (nSectors > 0)
    {
        UINT iCylinder =  Sector / (pdi->nHeads * pdi->nSectors);
        UINT iHead     = (Sector / pdi->nSectors) % pdi->nHeads;
        UINT iSector   = (Sector % pdi->nSectors);
        UINT count     = pdi->nSectors - iSector;

        if (count > nSectors) count = nSectors;

        regs.h.cl = (UCHAR)(((iSector + 1) & 0x3f) | ((iCylinder >> 2) & 0xC0));
        regs.h.ch = (UCHAR)iCylinder;
        regs.h.dh = (UCHAR)iHead;
        regs.h.dl = Drive;
        regs.h.al = count;
        regs.h.ah = 2;
        regs.x.es = SEG( Buffer );
        regs.x.bx = OFS( Buffer );

        int86( 0x13, &regs, &regs );

        if ((regs.x.cflag) || (regs.h.ah != 0))
        {
            break;
        }

        /* Update counts */
        nSectors -= count;
        Sector   += count;
        Read     += count;
        Buffer    = (CHAR*)Buffer + count * pdi->nBytesPerSector;
    }

    return Read;
}
