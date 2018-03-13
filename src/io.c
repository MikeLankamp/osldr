#include <drive.h>
#include <errno.h>
#include <io.h>
#include <stdlib.h>
#include <string.h>

#define MAX_READ_TRY    8   /* Try to a read a sector this many times at most */

/* Registered Filesystems */
BOOL FatMount( DEVICE* Device );
BOOL RawMount( DEVICE* Device );    /* Do not add this in the list */

typedef BOOL (*FSMOUNTFUNC)(DEVICE* pdi);

/* Alter this when adding or removing a filesystem to OSLDR */
static FSMOUNTFUNC FsMountFunctions[] = {
    FatMount,
    NULL
};

static DEVICE* Devices;
static ULONG   BootDevice;

VOID IoInitialize( ULONG device )
{
    BootDevice = device;
    Devices    = NULL;
}

static CHAR* ParseDevice( CHAR *path, ULONG* Device )
{
    CHAR* endptr;
    ULONG Partition = 0xFF;

    if (*path == '/')
    {
        /* We have no device, use boot device */
        *Device = BootDevice;
        return path;
    }

    /* Parse drive type */
    if (((path[0] != 'f') && (path[0] != 'h')) || (path[1] != 'd'))
    {
        /* We don't have "fd" or "hd" */
        errno = ENODEV;
        return NULL;
    }

    /* Parse drive number */
    *Device = strtoul( &path[2], &endptr, 10 );
    if ((endptr == &path[2]) || (*Device > 0x7F))
    {
        /* No or invalid drive number */
        if (errno == EZERO)
        {
            errno = ENODEV;
        }
        return NULL;
    }

    if (path[0] == 'h')
    {
        /* Its a hard drive, parse optional partition number */
        path = endptr;

        *Device |= 0x80;

        if (*path++ == ',')
        {
            /* Partition number present */
            Partition = strtoul( path, &endptr, 10 );
            if ((endptr == path) || (Partition > 254))
            {
                /* No or invalid partition number */
                if (errno == EZERO)
                {
                    errno = ENOPART;
                }
                return NULL;
            }

            /* FIXME: When we support BSD partitions, put code here */
        }
    }

    if ((*endptr != '/') && (*endptr != '\0'))
    {
        errno = ENOENT;
        return NULL;
    }

    /* Change to Multiboot compliant representation */
    *Device = (*Device << 24) | (Partition << 16) | 0xFFFF;

    return endptr;
}

#define PART_UNUSED     0x00
#define PART_EXTENDED1  0x05
#define PART_EXTENDED2  0x0F
#define PART_BOOTABLE   0x80

#define IS_EXTENDED(p)  (((p)->Type == PART_EXTENDED1) || ((p)->Type == PART_EXTENDED2))
#define IS_UNUSED(p)     ((p)->Type == PART_UNUSED)

typedef struct _PARTITION
{
    UCHAR Status;
    UCHAR StartCHS[3];
    UCHAR Type;
    UCHAR EndCHS[3];
    ULONG StartLBA;
    ULONG Size;
} PACKED PARTITION;

static DEVICE* OpenDevice( ULONG Device, BOOL MountFS )
{
    UCHAR     Drive = (Device >> 24) & 0xFF;
    UCHAR     Part1 = (Device >> 16) & 0xFF;
    UCHAR     Part2 = (Device >>  8) & 0xFF;
    UCHAR     Part3 = (Device >>  0) & 0xFF;
    ULONG     Start;
    ULONGLONG Size;
    DEVICE*   pdev;
    INT       i;

    /* Check if the device was opened before */
    for (pdev = Devices; pdev != NULL; pdev = pdev->Next)
    {
        if ((pdev->DeviceId == Device) && (pdev->hasFileSystem == MountFS))
        {
            return pdev;
        }
    }

    if ((Part2 != 0xFF) || (Part3 != 0xFF))
    {
        /* BSD partitions and who-knows-what-else are not supported */
        errno = ENOPART;
        return NULL;
    }

    DRIVE_INFO* pdi = GetDriveParameters( Drive );
    if (pdi == NULL)
    {
        /* Couldn't get drive parameters */
        errno = EIO;
        return NULL;
    }

    if (Part1 != 0xFF)
    {
        /* We have to find the wanted partition */
        PARTITION* parts;
        PARTITION* part = NULL;

        /* Read MBR */
        UCHAR* MBR = malloc( pdi->nBytesPerSector );
        if (MBR == NULL)
        {
            errno = ENOMEM;
            return NULL;
        }

        if (ReadDrive( Drive, 0, 1, MBR ) != 1)
        {
            free( MBR );
            errno = EIO;
            return NULL;
        }

        /* Find partition */
        parts = (PARTITION*)(MBR + 0x1BE);
        for (i = 0; i < 4; i++)
        {
            if (!IS_UNUSED(&parts[i]))
            {
                if (IS_EXTENDED(&parts[i]))
                {
                    /* Extended partition */
                    if (Part1 == i)
                    {
                        /* We want to read the extended partition, we can't do that */
                        break;
                    }

                    if (Part1 >= 4)
                    {
                        if (part != NULL)
                        {
                            /* Two extended partition entries, table's invalid */
                            break;
                        }
                        part = &parts[i];
                    }
                }
                else if (Part1 == i)
                {
                    /* This is simply the partition that we want */
                    part = &parts[i];
                }
            }
        }

        if ((i < 4) || (part == NULL))
        {
            /* Something went wrong or the partition could not be found */
            errno = ENOPART;
            free( MBR );
            return NULL;
        }

        if (Part1 >= 4)
        {
            /* Now we have to traverse the extended partition list */
            for (i = 4; part != NULL; i++)
            {
                if (ReadDrive( Drive, part->StartLBA, 1, MBR ) != 1)
                {
                    free( MBR );
                    errno = EIO;
                    return NULL;
                }

                part  = NULL;
                parts = (PARTITION*)(MBR + 0x1BE);
                if (IS_EXTENDED(&parts[0]))
                {
                    /* First partition entry isn't allowed to be extended */
                    break;
                }

                if (Part1 == i)
                {
                    /* We've found the partition we want */
                    part = &parts[0];
                    break;
                }

                if ((!IS_UNUSED(&parts[1])) && (IS_EXTENDED(&parts[1])))
                {
                    /* Go to next list item */
                    part = &parts[1];
                }
            } while (part != NULL);

            if (part == NULL)
            {
                /* We couldn't find the partition */
                free( MBR );
                errno = ENOPART;
                return NULL;
            }
        }

        /* Now part points to the partition entry we want */
        Start = part->StartLBA;
        Size  = part->Size;

        free( MBR );

        if (Start + Size > pdi->nTotalSectors)
        {
            /* Invalid partition */
            errno = ENOPART;
            return NULL;
        }
    }
    else
    {
        /* The entire drive is the partition */
        Start = 0;
        Size  = pdi->nTotalSectors;
    }

    /* Create the device object */
    pdev = malloc( sizeof(DEVICE) );
    if (pdev == NULL)
    {
        errno = ENOMEM;
        return NULL;
    }

    /* Initialize object */
    pdev->DeviceId      = Device;
    pdev->StartSector   = Start;
    pdev->nSectors      = Size;
    pdev->hasFileSystem = MountFS;
    pdev->Cache.nItems  = 0;
    pdev->Cache.Time    = 0;

    /* Mount device */
    if (MountFS)
    {
        for (i = 0; FsMountFunctions[i] != NULL; i++)
        {
            if (FsMountFunctions[i]( pdev ))
            {
                break;
            }
        }

        if (FsMountFunctions[i] == NULL)
        {
            free( pdev );

            errno = ENOFSYS;
            return NULL;
        }
    }
    else
    {
        /* Raw device */
        RawMount( pdev );
    }

    /* Add to linked list */
    pdev->Next = Devices;
    Devices    = pdev;

    return pdev;
}

static VOID* ReadCache( CACHE* Cache, ULONGLONG Tag )
{
    ULONG i;

    /* Search cache */
    for (i = 0; i < Cache->nItems; i++)
    {
        if (Cache->Items[i].Tag == Tag)
        {
            /* Update access time */
            Cache->Items[i].Time = Cache->Time++;

            /* Return data */
            return Cache->Items[i].Data;
        }
    }

    /* Entry not cached */
    return NULL;
}

static VOID WriteCache( CACHE* Cache, ULONGLONG Tag, VOID* Data )
{
    ULONG i = 0;

    if (Cache->nItems == CACHE_SIZE)
    {
        /* Cache is full, replace least recently used */
        ULONG j;

        /* Find and free Least Recently Used */
        for (j = 1; j < CACHE_SIZE; j++)
        {
            if (Cache->Items[j].Time < Cache->Items[i].Time)
            {
               i = j;
            }
        }
        free( Cache->Items[i].Data );
    }
    else
    {
        i = Cache->nItems++;
    }

    Cache->Items[i].Tag  = Tag;
    Cache->Items[i].Time = Cache->Time++;
    Cache->Items[i].Data = Data;
}

ULONGLONG ReadSector( DEVICE* Device, ULONGLONG Sector, ULONGLONG nSectors, VOID* Buffer )
{
    ULONGLONG   Read  = 0;
    UCHAR       Drive = Device->DeviceId >> 24;
    DRIVE_INFO* pdi   = GetDriveParameters( Drive );

    if (pdi == NULL)
    {
        errno = EIO;
        return 0;
    }

    if (Sector + nSectors > Device->nSectors)
    {
        /* Invalid sector and/or count */
        errno = EIO;
        return 0;
    }

    while (nSectors > 0)
    {
        VOID*     tmpbuf;
        ULONGLONG count = nSectors;

        /* See if the current sector is cached */
        tmpbuf = ReadCache( &Device->Cache, Sector );
        if (tmpbuf != NULL)
        {
            /* Yes, copy contents */
            memcpy( Buffer, tmpbuf, pdi->nBytesPerSector );
            count = 1;
        }
        else
        {
            /* No, read sectors from disk and cache */
            INT       i;
            ULONGLONG end = Sector + 1;

            /* See how many consecutive sectors aren't cached */
            for (count = 1; end < Sector + nSectors; end++, count++)
            {
                if (ReadCache( &Device->Cache, end ) != NULL)
                {
                    break;
                }
            }

            /* Allocate as much sectors as possible */
            do
            {
                tmpbuf = malloc( count * pdi->nBytesPerSector );
                if (tmpbuf != NULL)
                {
                    break;
                }

                /* We can't allocate this much, try half as much */
                count /= 2;
            } while (count > 1);

            if (tmpbuf == NULL)
            {
                /* Not enough memory for even one sector */
                errno = ENOMEM;
                break;
            }

            for (i = 0; i < MAX_READ_TRY; i++)
            {
                if (ReadDrive( Drive, Device->StartSector + Sector, count, tmpbuf ) == count)
                {
                    break;
                }
                ResetDrive( Drive );
            }

            if (i == MAX_READ_TRY)
            {
                /* The read failed */
                errno = EIO;
                free( tmpbuf );
                break;
            }

            /* Write to cache */
            for (i = 0; i < count; i++)
            {
                VOID* secbuf = malloc( pdi->nBytesPerSector );
                if (secbuf != NULL)
                {
                    /* Only write to cache if we can allocate memory */
                    memcpy( secbuf, (CHAR*)tmpbuf + i * pdi->nBytesPerSector, pdi->nBytesPerSector );
                    WriteCache( &Device->Cache, Sector + i, secbuf );
                }
            }

            /* Copy read data to user buffer */
            memcpy( Buffer, tmpbuf, count * pdi->nBytesPerSector );
            free( tmpbuf );
        }

        /* Adjust values */
        nSectors -= count;
        Sector   += count;
        Read     += count;
        Buffer    = (CHAR*)Buffer + (count * pdi->nBytesPerSector);
    }

    return Read;
}

BOOL IsDevice( FILE* file )
{
    return file->IsDevice;
}

FILE* OpenFile( CHAR* Path )
{
    ULONG   DeviceId;
    DEVICE* pdev;
    FILE*   File;

    Path = ParseDevice( Path, &DeviceId );
    if (Path == NULL)
    {
        return NULL;
    }

    pdev = OpenDevice( DeviceId, (*Path != '\0') );
    if (pdev == NULL)
    {
        return NULL;
    }

    File = pdev->OpenFile( pdev, Path );
    if (File != NULL)
    {
        File->Device   = pdev;
        File->IsDevice = (*Path == '\0');
    }

    return File;
}

ULONGLONG ReadFile( FILE* File, VOID* Buffer, ULONGLONG nBytes )
{
    return File->Device->ReadFile( File, Buffer, nBytes );
}

ULONGLONG GetFileSize( FILE* File )
{
    return File->Device->GetFileSize( File );
}

BOOL SetFilePointer( FILE* File, LONGLONG Offset, INT From)
{
    return File->Device->SetFilePointer( File, Offset, From );
}

ULONGLONG GetFilePointer( FILE* File )
{
    return File->Device->GetFilePointer( File );
}

VOID CloseFile( FILE* File )
{
    File->Device->CloseFile( File );
    free( File );
}
