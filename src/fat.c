#include <ctype.h>
#include <drive.h>
#include <errno.h>
#include <io.h>
#include <stdlib.h>
#include <string.h>

#define FAT_VERSION_HIGH    0
#define FAT_VERSION_LOW     0

#define MAX_READ_TRY        8

typedef struct _BPB
{
    UCHAR  BS_jmpBoot[3];
    CHAR   BS_OEMName[8];
    USHORT BPB_BytsPerSec;
    UCHAR  BPB_SecPerClus;
    USHORT BPB_RsvdSecCnt;
    UCHAR  BPB_NumFATs;
    USHORT BPB_RootEntCnt;
    USHORT BPB_TotSec16;
    UCHAR  BPB_Media;
    USHORT BPB_FATSz16;
    USHORT BPB_SecPerTrk;
    USHORT BPB_NumHeads;
    ULONG  BPB_HiddSec;
    ULONG  BPB_TotSec32;

    /* FAT specific data */
    union _FAT
    {
        struct _FAT12_16
        {
            UCHAR  BS_DrvNum;
            UCHAR  BS_Reserved1;
            UCHAR  BS_BootSig;
            ULONG  BS_VolID;
            CHAR   BS_VolLab[11];
            CHAR   BS_FilSysType[8];
        } FAT12_16;

        struct _FAT32
        {
            ULONG  BPB_FATSz32;
            USHORT BPB_ExtFlags;
            USHORT BPB_FSVer;
            ULONG  BPB_RootClus;
            USHORT BPB_FSInfo;
            USHORT BPB_BkBootSec;
            UCHAR  BPB_Reserved[12];
            UCHAR  BS_DrvNum;
            UCHAR  BS_Reserved1;
            UCHAR  BS_BootSig;
            ULONG  BS_VolID;
            CHAR   BS_VolLab[11];
            CHAR   BS_FilSysType[8];
        } FAT32;
    } FAT;

} PACKED BPB;

/* FAT specific information */
typedef struct _FAT_INFO
{
    /* Some pre-calculated area offsets */
    ULONG  nDataSectors;
    ULONG  FatStart;
    ULONG  RootDirStart;
    ULONG  DataStart;
    ULONG  BytsPerClus;
    ULONG  FatType;
    ULONG  EndCluster;

    /* The BPB from the bootsector */
    BPB*   Bpb;

} FAT_INFO;

typedef struct _DIR_ENTRY
{
    CHAR   DIR_Name[11];
    UCHAR  DIR_Attr;
    UCHAR  DIR_NTRes;
    UCHAR  DIR_CrtTimeTenth;
    USHORT DIR_CrtTime;
    USHORT DIR_CrtDate;
    USHORT DIR_LstAccDate;
    USHORT DIR_FstClusHI;
    USHORT DIR_WrtTime;
    USHORT DIR_WrtDate;
    USHORT DIR_FstClusLO;
    ULONG  DIR_FileSize;
} PACKED DIR_ENTRY;

#define ATTR_READ_ONLY       1
#define ATTR_HIDDEN          2
#define ATTR_SYSTEM          4
#define ATTR_VOLUME_ID       8
#define ATTR_DIRECTORY      16
#define ATTR_ARCHIVE        32
#define ATTR_LONG_NAME      (ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID)
#define ATTR_LONG_NAME_MASK (ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID | ATTR_DIRECTORY | ATTR_ARCHIVE)

typedef struct _LDIR_ENTRY
{
    UCHAR  LDIR_Ord;
    WCHAR  LDIR_Name1[5];
    UCHAR  LDIR_Attr;
    UCHAR  LDIR_Type;
    UCHAR  LDIR_Chksum;
    WCHAR  LDIR_Name2[6];
    USHORT LDIR_FstClusLO;
    WCHAR  LDIR_Name3[2];
} PACKED LDIR_ENTRY;

#define LAST_LONG_ENTRY 0x40

/* Maximum entries per short entry */
#define MAX_ENTRY_VALUE 0x3F

/* Maximum long filename length (though there are 63 entries allowed) */
#define MAX_LONG_FILENAME_LENGTH 0xFF

/* File structure */
typedef struct _FILE_INFO
{
    /* Used by the I/O Manager */
    FILE       General;

    /* File information */
    DIR_ENTRY  Info;

    /* Current file cursor */
    ULONGLONG  Cursor;
    ULONG      Cluster;
    
} FILE_INFO;

/* Removes all leading and trailing spaces and periods */
static CHAR* Trim( CHAR* src )
{
    CHAR* end;

    while (*src == ' ') src++;
    end = strchr( src, '\0' );
    while ((end > src) && ((*(end-1) == ' ') || (*(end-1) == '.'))) end--;
    *end = '\0';

    return src;
}

static BOOL ValidateFilename( CHAR* Filename )
{
    if (strlen( Filename ) <= 255)
    {
        while (*Filename != '\0')
        {
            CHAR ch = *Filename++;

            /* Check for illegal characters */
            if ((ch < 32) || ((ch < 127) &&
                ((ch == '\\') || (ch == '/') || (ch == ':') ||
                 (ch == '*')  || (ch == '?') || (ch == '"') ||
                 (ch == '<')  || (ch == '>') || (ch == '|'))))
            {
                return FALSE;
            }
        }
        return TRUE;
    }
    return FALSE;
}

/*
 * Converts an string to an OEM string with short-name characters.
 * Should the resulting short-name be invalid, Dest will be an empty string.
 */
static VOID UnicodeToOem( CHAR* Dest, CHAR* Src )
{
    CHAR* dot = strchr( Src, '.');
    ULONG i   = 0;

    if ((dot == NULL) || (dot - Src <= 8))
    {
        for (; i < 11; i++)
        {
            CHAR ch = *Src & 0xFF;

            #if 0
            /* Special Non-OEM Unicode equivalent characters are invalid */
            if (*Src > 0x00FF)
            {
                i = 0;
                break;
            }
            #endif

            if (ch == '.')
            {
                if (Src == dot)
                {
                    if (i == 8)
                    {
                        /* Exact fit, add nothing */
                        Src++;
                        i--;
                    }
                    else if (i < 8)
                    {
                        Dest[i] = ' ';  /* Pad with spaces */
                    }
                }
                else
                {
                    /* '.' detected in extension. This is invalid */
                    i = 0;
                    break;
                }
            }
            else if (ch == '\0')
            {
                Dest[i] = ' ';  /* Pad with spaces */
            }
            else
            {
                /* Uppercase character */
                ch = toupper( ch );

                /* Check for illegal characters */

                if ((!isalnum( ch )) &&
                    (ch != '$') && (ch != '%') && (ch != '\'') && (ch > 0) &&
                    (ch != '-') && (ch != '_') && (ch != '@') && (ch != '~') &&
                    (ch != '`') && (ch != '!') && (ch != '(') && (ch != ')') &&
                    (ch != '{') && (ch != '}') && (ch != '^') && (ch != '#') &&
                    (ch != '&'))
                {
                    i = 0;
                    break;
                }

                Dest[i] = ch;
                Src++;
            }
        }
    }

    Dest[i] = '\0';
}

/* Calculates the checksum of an 11 char DOS filename */
static UCHAR CheckSum( CHAR* pFcbName )
{
    SHORT FcbNameLen;
    UCHAR Sum = 0;

    for (FcbNameLen = 11; FcbNameLen != 0; FcbNameLen--)
    {
        Sum = ((Sum & 1) << 7) + (Sum >> 1) + *pFcbName++;
    }
    return Sum;
}

/* Determines the next cluster based on FAT contents */
static ULONG GetNextCluster( DEVICE* Device, ULONG Cluster )
{
    FAT_INFO* pfi  = (FAT_INFO*)Device->Data;
    ULONG     Res  = 0xFFFFFFFF;
    ULONG     Clus = 0;
    ULONG     Ofs;
    UCHAR*    Buffer;

    /* Calculate cluster / offset */
    switch (pfi->FatType)
    {
        case 12: Clus = Cluster + (Cluster / 2); break;
        case 16: Clus = Cluster * 2;             break;
        case 32: Clus = Cluster * 4;             break;
    }

    Ofs  = Clus % pfi->Bpb->BPB_BytsPerSec;
    Clus = Clus / pfi->Bpb->BPB_BytsPerSec;

    /* Read the FAT sector */
    Buffer = malloc( pfi->Bpb->BPB_BytsPerSec );
    if (Buffer == NULL)
    {
        return Res;
    }

    if (ReadSector( Device, pfi->FatStart + Clus, 1, Buffer ) == 1)
    {
        switch (pfi->FatType)
        {
            case 12:
                if (Ofs == pfi->Bpb->BPB_BytsPerSec - 1)
                {
                    /* FAT12 entry spans across cluster boundry */
                    Res = Buffer[ Ofs ];

                    /* Read the next FAT sector */
                    if (ReadSector( Device, pfi->FatStart + Clus + 1, 1, Buffer ) == 1)
                    {
                        Res |= (Buffer[ 0 ] << 8);
                    }
                    else
                    {
                        Res = 0xFFFF;
                    }
                }
                else
                {
                    Res = *(USHORT*)(Buffer + Ofs);
                }
                Res = (Cluster & 1) ? (Res >> 4) : (Res & 0xFFF);
                break;

            case 16:
                Res = *(USHORT*)(Buffer + Ofs);
                break;

            case 32:
                Res = *(ULONG*)(Buffer + Ofs);
                break;
        }
    }

    free( Buffer );
    return Res;
}

static BOOL FindDirEntry( DEVICE* Device, ULONG Cluster, CHAR* Path, DIR_ENTRY* entry )
{
    CHAR      OemFilename[12];
    CHAR      Filename[256];
    FAT_INFO* pfi    = (FAT_INFO*)Device->Data;
    UCHAR*    Buffer = NULL;
    UINT      nRootEntries = 0;
    ULONG     Sector = 0;
    CHAR*     Name;

    /* For parsing Long Filename Entries */
    UINT  LastDirEntry = 0;
    UINT  DirEntry     = 0;
    UCHAR Checksum     = 0;

    /* Duplicate path */
    Name = malloc( strlen( Path ) + 1 );
    if (Name == NULL)
    {
        errno = ENOMEM;
        return FALSE;
    }
    strcpy( Name, Path );

    /* Trim and validate long name */
    Name = Trim( Name );
    if (!ValidateFilename( Name ))
    {
        errno = ENOENT;
        free( Name );
        return FALSE;
    }

    Buffer = malloc( pfi->BytsPerClus );
    if (Buffer == NULL)
    {
        errno = ENOMEM;
        free( Name );
        return FALSE;
    }

    /* Transform to OEM string for check with short filename */
    UnicodeToOem( OemFilename, Name );

    /* The root dir start sector is set in BPB */
    if (Cluster < 2)
    {
        Sector = pfi->RootDirStart;
    }

    while (Cluster < pfi->EndCluster)
    {
        INT i;

        /* Calculate start cluster */
        if (Cluster >= 2)
        {
            /* Non-root directories */
            Sector = pfi->DataStart + (Cluster-2) * pfi->Bpb->BPB_SecPerClus;
        }

        /* Read the cluster */
        if (ReadSector( Device, Sector, pfi->Bpb->BPB_SecPerClus, Buffer ) != pfi->Bpb->BPB_SecPerClus)
        {
            break;
        }

        /* Iterate over all directory entries in the cluster */
        for (i = 0; ((Cluster >= 2) || (nRootEntries < pfi->Bpb->BPB_RootEntCnt)) && (i < pfi->BytsPerClus); i += sizeof(DIR_ENTRY), nRootEntries++)
        {
            DIR_ENTRY* pde = (DIR_ENTRY*)&Buffer[ i ];

            if (pde->DIR_Name[0] == '\0')
            {
                /* End of directory */
                break;
            }

            if ((UCHAR)pde->DIR_Name[0] == 0xe5)
            {
                /* Deleted file, ignore */
                continue;
            }

            if ((UCHAR)pde->DIR_Name[0] == 0x05)
            {
                /* Some Kanji char replacement */
                *(UCHAR*)&pde->DIR_Name[0] = 0xE5;
            }

            if ((pde->DIR_Attr & ATTR_LONG_NAME_MASK) == ATTR_LONG_NAME)
            {
                /* This is a long filename entry */
                LDIR_ENTRY* plde = (LDIR_ENTRY*)pde;

                /* Check for correct use of the LAST_LONG_ENTRY flag */
                if ((DirEntry != LastDirEntry) ^ ((plde->LDIR_Ord & LAST_LONG_ENTRY) != 0))
                {
                    /* Correct use of flag */
                    if (LastDirEntry == 0)
                    {
                        /* First entry in a set */
                        LastDirEntry = plde->LDIR_Ord &= ~LAST_LONG_ENTRY;
                        Checksum     = plde->LDIR_Chksum;
                        DirEntry     = LastDirEntry;
                    }

                    if ((DirEntry > 0) && (DirEntry <= MAX_ENTRY_VALUE) &&
                        (plde->LDIR_Ord == DirEntry) &&
                        (plde->LDIR_FstClusLO == 0) &&
                        (plde->LDIR_Type == 0) &&
                        (plde->LDIR_Chksum == Checksum))
                    {
                        /* Valid directory entry, store filename */
                        CHAR* name = &Filename[ --DirEntry * 13 ];

                        *name++ = plde->LDIR_Name1[0];
                        *name++ = plde->LDIR_Name1[1];
                        *name++ = plde->LDIR_Name1[2];
                        *name++ = plde->LDIR_Name1[3];
                        *name++ = plde->LDIR_Name1[4];
                        *name++ = plde->LDIR_Name2[0];
                        *name++ = plde->LDIR_Name2[1];
                        *name++ = plde->LDIR_Name2[2];

                        if (name < &Filename[255])
                        {
                            /* These could overflow the 255 limit */
                            *name++ = plde->LDIR_Name2[3];
                            *name++ = plde->LDIR_Name2[4];
                            *name++ = plde->LDIR_Name2[5];
                            *name++ = plde->LDIR_Name3[0];
                            *name++ = plde->LDIR_Name3[1];
                        }

                        if (DirEntry == LastDirEntry - 1)
                        {
                            /* Terminate string for last entry */
                            *name = '\0';
                        }

                        /* Indicate success */
                        plde = NULL;
                    }
                }

                if (plde != NULL)
                {
                    /* Invalid ordinal, reset */
                    LastDirEntry = 0;
                    DirEntry     = 0;
                }
            }
            else
            {
                /* This is the short filename entry */
                if (LastDirEntry != 0)
                {
                    /* Short name with long name entries */
                    if ((DirEntry != 0) || (CheckSum( pde->DIR_Name ) != Checksum))
                    {
                        /* Invalid LFN entries, ignore them */
                        LastDirEntry = 0;
                    }
                    else if (stricmp( Filename, Name ) != 0)
                    {
                        /* Not the entry we want */
                        pde = NULL;
                    }
                }

                if (LastDirEntry == 0)
                {
                    /* Short name without long name entries */
                    if (strncmp(OemFilename, pde->DIR_Name, 11) != 0)
                    {
                        /* Not the entry we want */
                        pde = NULL;
                    }
                }

                if (pde != NULL)
                {
                    /* Found the right entry */
                    *entry = *pde;
                    free( Name );
                    free( Buffer );
                    return TRUE;
                }

                /* Reset LFN vars on a short filename entry */
                LastDirEntry = 0;
                DirEntry     = 0;
            }
        }

        if (i < pfi->BytsPerClus)
        {
            /* The loop was aborted, abort this one too */
            break;
        }

        /* Get next cluster */
        if (Cluster < 2)
        {
            /* Root dir? Simply read the next cluster */
            Sector += pfi->Bpb->BPB_SecPerClus;
        }
        else
        {
            /* Else, use the FAT */
            Cluster = GetNextCluster( Device, Cluster );
        }
    }

    /* Entry not found */
    errno = ENOENT;
    free( Name );
    free( Buffer );
    return FALSE;
}

static FILE* _OpenFile(DEVICE* Device, CHAR* Path )
{
    FAT_INFO*  pfi     = (FAT_INFO*)Device->Data;
    ULONG      Cluster = (pfi->FatType == 32) ? pfi->Bpb->FAT.FAT32.BPB_RootClus : 0;
    FILE_INFO* file;
    UCHAR      attr;
    DIR_ENTRY  de;
    CHAR*      p;
    BOOL       ret;

    if (*Path++ != '/')
    {
        errno = ENOENT;
        return NULL;
    }

    do
    {
        p = strchr(Path, '/');
        if (p != NULL) *p = '\0';

        ret = FindDirEntry( Device, Cluster, Path, &de );

        if (p != NULL) *p = '/';
        Path = p + 1;

        /* Set active cluster */
        Cluster = MAKELONG(de.DIR_FstClusHI, de.DIR_FstClusLO);

        attr = de.DIR_Attr & (ATTR_DIRECTORY | ATTR_VOLUME_ID);

    } while ((ret) && (attr == ATTR_DIRECTORY) && (p != NULL));

    if ((ret) && (attr == 0) && (p == NULL))
    {
        /* We found a file, create and init FILE structure */
        file = malloc( sizeof(FILE_INFO) );
        if (file == NULL)
        {
            errno = ENOMEM;
            return NULL;
        }

        file->Info    = de;
        file->Cursor  = 0;
        file->Cluster = MAKELONG(de.DIR_FstClusHI, de.DIR_FstClusLO);

        return (FILE*)file;
    }

    if (ret)
    {
        /* We found a unexpected file or volume descriptor */
        errno = ENOENT;
    }

    return NULL;
}

static ULONGLONG _GetFileSize( FILE* File )
{
    FILE_INFO* file = (FILE_INFO*)File;
    return file->Info.DIR_FileSize;
}

static ULONGLONG _ReadFile(FILE* file, VOID* Buffer, ULONGLONG nBytes)
{
    FILE_INFO* File = (FILE_INFO*)file;
    FAT_INFO*  pfi  = (FAT_INFO*)file->Device->Data;
    ULONGLONG  Read = 0;

    /* Adjust nBytes to prevent reading past EOF */
    if (File->Cursor + nBytes > File->Info.DIR_FileSize )
    {
        nBytes = File->Info.DIR_FileSize - File->Cursor;
    }

    while ((File->Cluster >= 2) && (File->Cluster < pfi->EndCluster) && (Read < nBytes))
    {
        ULONGLONG nextClus  = (File->Cursor % pfi->BytsPerClus) + nBytes;
        ULONGLONG len       = (nextClus + pfi->BytsPerClus - 1) / pfi->BytsPerClus;
        ULONGLONG nClusters = 0;

        /* Find out how long this run of consecutive clusters is */
        do
        {
            nextClus = GetNextCluster( file->Device, File->Cluster + nClusters++ );
        } while ((nextClus >= 2) && (nextClus < pfi->EndCluster) && /* Valid cluster? */
                 (nextClus == File->Cluster + nClusters) &&         /* Next cluster?  */
                 (nextClus  < File->Cluster + len));                /* Stop after nBytes */

        /* Read the entire run of clusters */
        while (nClusters > 0)
        {
            ULONGLONG nClus  = nClusters;
            ULONGLONG Sector = pfi->DataStart + (File->Cluster - 2) * pfi->Bpb->BPB_SecPerClus;
            CHAR*     ClusBuf;
            CHAR*     src;

            /* If we can't allocate the entire run at once, break it up
             * into smaller pieces. Hence the above while loop */
            do
            {
                ClusBuf = malloc( nClus * pfi->BytsPerClus );
                if (ClusBuf != NULL)
                {
                    break;
                }
                nClus /= 2;
            } while (nClus > 1);

            if (ClusBuf == NULL)
            {
                /* Error, return what we read so far */
                errno = ENOMEM;
                return Read;
            }

            if (ReadSector( file->Device, Sector, nClus * pfi->Bpb->BPB_SecPerClus, ClusBuf ) != nClus * pfi->Bpb->BPB_SecPerClus)
            {
                /* Error, return what we read so far */
                errno = EIO;
                free( ClusBuf );
                return Read;
            }

            /* Copy the full clusters by default */
            src = ClusBuf;
            len = nClus * pfi->BytsPerClus;

            if (File->Cursor % pfi->BytsPerClus != 0)
            {
                /* Read starting unaligned bytes */
                src += File->Cursor % pfi->BytsPerClus;
                len -= File->Cursor % pfi->BytsPerClus;
            }

            if (nBytes < len)
            {
                /* Read remaining unaligned bytes */
                len = nBytes;
            }

            memcpy( Buffer, src, len );
            Buffer         = (CHAR*)Buffer + len;
            Read          += len;
            nBytes        -= len;
            File->Cluster += (File->Cursor + len) / pfi->BytsPerClus - File->Cursor / pfi->BytsPerClus;
            File->Cursor  += len;
            nClusters     -= nClus;

            free( ClusBuf );
        }

        if (File->Cursor % pfi->BytsPerClus == 0)
        {
            File->Cluster = nextClus;
        }
    }

    return Read;
}

static ULONGLONG _GetFilePointer( FILE* File )
{
    return ((FILE_INFO*)File)->Cursor;
}

static BOOL _SetFilePointer( FILE* file, LONGLONG offset, INT whence )
{
    FILE_INFO* File = (FILE_INFO*)file;
    FAT_INFO*  pfi  = file->Device->Data;

    LONGLONG Cursor = offset;
    if (whence == FILE_CURRENT)
    {
        Cursor += File->Cursor;
    }
    else if (whence == FILE_END)
    {
        Cursor = File->Info.DIR_FileSize - Cursor;
    }

    /* Validate new cursor */
    if ((Cursor < 0) || (Cursor >= File->Info.DIR_FileSize))
    {
        return FALSE;
    }

    /* We need to find the cluster for the new cursor.
     * If the new cursor comes after the current cursor, no problem, just read
     * FAT chain from here on. If it comes before, we have to start from 0.
     */

    /* Align old cursor to start of cluster for ease */
    File->Cursor = File->Cursor & -pfi->BytsPerClus;

    if (Cursor < File->Cursor)
    {
        File->Cursor  = 0;
        File->Cluster = File->Info.DIR_FstClusLO;
    }

    while (File->Cursor + pfi->BytsPerClus <= Cursor)
    {
        File->Cluster = GetNextCluster( file->Device, File->Cluster );
        File->Cursor += pfi->BytsPerClus;
    }

    /* We're in the correct cluster now */
    File->Cursor = Cursor;

    return TRUE;
}

static VOID _CloseFile( FILE* File )
{
    /* We have no FAT-specific stuff to free */
}

/* Destroys all FAT private data */
static VOID _Release( DEVICE* Device )
{
    FAT_INFO* pfi = (FAT_INFO*)Device->Data;

    free( pfi->Bpb );
    free( Device->Data );
}

/* Check if the partition is FAT and initialize private data if so */
BOOL FatMount( DEVICE* Device )
{
    BPB*   pBpb;
    ULONG  i;
    INT    FatType = 0; /* Still undetermined: possibly 12, 16 or 32 */
    ULONG  TotSec;
    ULONG  FATSz;
    ULONG  DataSec;
    ULONG  RootDirSectors;
    ULONG  nClusters;
    UCHAR* Buffer;

    FAT_INFO*       fi;
    DRIVE_INFO*     pdi;

    pdi = GetDriveParameters( Device->DeviceId >> 24 );
    if (pdi == NULL)
    {
        return FALSE;
    }

    /* Read BPB */
    Buffer = malloc( pdi->nBytesPerSector );
    if (Buffer == NULL)
    {
        return FALSE;
    }

    for (i = 0; i < MAX_READ_TRY; i++)
    {
        if (ReadSector( Device, 0, 1, Buffer ) == 1)
        {
            break;
        }
        ResetDrive( Device->DeviceId >> 24 );
    }
    if (i == MAX_READ_TRY)
    {
        free( Buffer );
        return FALSE;
    }

    /*
     * Check values for FAT
     */
    pBpb = (BPB*)Buffer;

    /* Check legal values */
    if (((pBpb->BPB_BytsPerSec !=  512) && (pBpb->BPB_BytsPerSec != 1024) &&
         (pBpb->BPB_BytsPerSec != 2048) && (pBpb->BPB_BytsPerSec != 4096)) ||
        ((pBpb->BPB_SecPerClus !=   1) && (pBpb->BPB_SecPerClus !=   2) &&
         (pBpb->BPB_SecPerClus !=   4) && (pBpb->BPB_SecPerClus !=   8) &&
         (pBpb->BPB_SecPerClus !=  16) && (pBpb->BPB_SecPerClus !=  32) &&
         (pBpb->BPB_SecPerClus !=  64) && (pBpb->BPB_SecPerClus != 128)) ||
        ((ULONG)pBpb->BPB_SecPerClus * (ULONG)pBpb->BPB_BytsPerSec > 32768) ||
        (pBpb->BPB_RsvdSecCnt == 0) ||
        (pBpb->BPB_BytsPerSec != pdi->nBytesPerSector))
    {
        free( Buffer );
        return FALSE;
    }

    /* If its greater than 1, it must be FAT32, otherwise undetermined */
    if (pBpb->BPB_RsvdSecCnt > 1)
    {
        FatType = 32;
    }

    /* Check Root Entry Count */
    if (pBpb->BPB_RootEntCnt == 0)
    {
        FatType = 32;
    }
    else if (FatType == 32)
    {
        free( Buffer );
        return FALSE;
    }
    else
    {
        /* FAT12 or FAT16 */
        FatType = 14;
    }
    /* Now we know for sure if its FAT12/16 or FAT32 */

    /* Validate values with FAT type */
    if (((pBpb->BPB_TotSec16 == 0) && (pBpb->BPB_TotSec32 == 0)) ||
        ((pBpb->BPB_TotSec16 != 0) && (FatType == 32)) ||
        ((pBpb->BPB_Media < 0xF8) && (pBpb->BPB_Media != 0xF0)) ||
        ((pBpb->BPB_FATSz16 == 0) ^ (FatType == 32)))
    {
        free( Buffer );
        return FALSE;
    }

    /* Check some more stuff for FAT32 */
    if ((FatType == 32) && ((pBpb->FAT.FAT32.BPB_FATSz32 == 0) ||
        (pBpb->FAT.FAT32.BPB_FSVer != ((FAT_VERSION_HIGH << 8) | FAT_VERSION_LOW))))
    {
        free( Buffer );
        return FALSE;
    }

    /* This is definitely FAT, check FAT type */
    RootDirSectors = ((pBpb->BPB_RootEntCnt * 32) + (pBpb->BPB_BytsPerSec-1)) / pBpb->BPB_BytsPerSec;
    FATSz     = (pBpb->BPB_FATSz16  != 0) ? pBpb->BPB_FATSz16  : pBpb->FAT.FAT32.BPB_FATSz32;
    TotSec    = (pBpb->BPB_TotSec16 != 0) ? pBpb->BPB_TotSec16 : pBpb->BPB_TotSec32;
    DataSec   = TotSec - (pBpb->BPB_RsvdSecCnt + (pBpb->BPB_NumFATs * FATSz) + RootDirSectors);
    nClusters = DataSec / pBpb->BPB_SecPerClus;

    /* Initialize private data */
    fi = malloc( sizeof(FAT_INFO) );
    if (fi == NULL)
    {
        errno = ENOMEM;
        free( Buffer );
        return FALSE;
    }

    if (nClusters < 4085)      { fi->FatType = 12; fi->EndCluster = 0x00000FF7; }
    else if(nClusters < 65525) { fi->FatType = 16; fi->EndCluster = 0x0000FFF7; }
    else
    {
        /* Check FAT32 version */
        if (pBpb->FAT.FAT32.BPB_FSVer != 0)
        {
            /* We only support version 0:0 */
            return FALSE;
        }

        fi->FatType = 32;
        fi->EndCluster = 0x0FFFFFF7;
    }

    fi->nDataSectors = DataSec;
    fi->FatStart     = pBpb->BPB_RsvdSecCnt;
    fi->RootDirStart = fi->FatStart + (pBpb->BPB_NumFATs * FATSz);
    fi->DataStart    = fi->RootDirStart + RootDirSectors;
    fi->BytsPerClus  = pBpb->BPB_BytsPerSec * pBpb->BPB_SecPerClus;
    fi->Bpb          = pBpb;

    Device->OpenFile       = _OpenFile;
    Device->SetFilePointer = _SetFilePointer;
    Device->GetFilePointer = _GetFilePointer;
    Device->GetFileSize    = _GetFileSize;
    Device->ReadFile       = _ReadFile;
    Device->CloseFile      = _CloseFile;
    Device->Release        = _Release;
    Device->Data           = fi;

    return TRUE;
}

