#include <drive.h>
#include <errno.h>
#include <io.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct _RAWFILE
{
    FILE      General;
    ULONGLONG Cursor;
} RAWFILE;

static FILE* _OpenFile( DEVICE* Device, CHAR* Path )
{
    RAWFILE* File = malloc( sizeof(RAWFILE) );
    if (File != NULL)
    {
        File->Cursor = 0;
    }
    return (FILE*)File;
}

static ULONGLONG _ReadFile( FILE* file, VOID* Buffer, ULONGLONG nBytes )
{
    RAWFILE*    File = (RAWFILE*)file;
    DRIVE_INFO* pdi  = GetDriveParameters( file->Device->DeviceId >> 24 );
    ULONG       Read = 0;

    while (nBytes > 0)
    {
        ULONGLONG nSectors;
        ULONGLONG Sector;
        ULONGLONG len;
        CHAR*     TmpBuf;
        CHAR*     src;

        nSectors = File->Cursor % pdi->nBytesPerSector + nBytes;
        nSectors = (nSectors + pdi->nBytesPerSector - 1) / pdi->nBytesPerSector;
        Sector   = File->Cursor / pdi->nBytesPerSector;

        /* If we can't allocate the entire run at once, break it up
         * into smaller pieces. Hence the above while loop */
        do
        {
            TmpBuf = malloc( nSectors * pdi->nBytesPerSector );
            if (TmpBuf != NULL)
            {
                break;
            }
            nSectors /= 2;
        } while (nSectors > 1);

        if (TmpBuf == NULL)
        {
            /* Error, return what we read so far */
            errno = ENOMEM;
            return Read;
        }

        if (ReadSector( file->Device, Sector, nSectors, TmpBuf ) != nSectors)
        {
            /* Error, return what we read so far */
            errno = EIO;
            free( TmpBuf );
            return Read;
        }

        /* Copy the full sectors by default */
        src = TmpBuf;
        len = nSectors * pdi->nBytesPerSector;

        if (File->Cursor % pdi->nBytesPerSector != 0)
        {
            /* Read starting unaligned bytes */
            src += File->Cursor % pdi->nBytesPerSector;
            len -= File->Cursor % pdi->nBytesPerSector;
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
        File->Cursor  += len;

        free( TmpBuf );
    }

    return Read;
}

static ULONGLONG _GetFileSize( FILE* File )
{
    DRIVE_INFO* pdi = GetDriveParameters( File->Device->DeviceId >> 24 );

    return File->Device->nSectors * pdi->nBytesPerSector;
}

static BOOL _SetFilePointer( FILE* file, LONGLONG Offset, INT From)
{
    RAWFILE*    File = (RAWFILE*)file;
    DRIVE_INFO* pdi   = GetDriveParameters( file->Device->DeviceId >> 24 );

    LONGLONG Cursor = Offset;
    if (From == FILE_CURRENT)
    {
        Cursor += File->Cursor;
    }
    else if (From == FILE_END)
    {
        Cursor = file->Device->nSectors * pdi->nBytesPerSector - Cursor;
    }

    /* Validate new cursor */
    if (Cursor >= file->Device->nSectors * pdi->nBytesPerSector)
    {
        return FALSE;
    }

    File->Cursor = Cursor;

    return TRUE;
}

static ULONGLONG _GetFilePointer( FILE* File )
{
    return ((RAWFILE*)File)->Cursor;
}

static VOID _CloseFile( FILE* File )
{
}

static VOID _Release( DEVICE* Device )
{
}

BOOL RawMount( DEVICE* Device )
{
    Device->OpenFile       = _OpenFile;
    Device->SetFilePointer = _SetFilePointer;
    Device->GetFilePointer = _GetFilePointer;
    Device->GetFileSize    = _GetFileSize;
    Device->ReadFile       = _ReadFile;
    Device->CloseFile      = _CloseFile;
    Device->Release        = _Release;

    return TRUE;
}
