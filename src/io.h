#ifndef IO_H
#define IO_H

#include <types.h>

typedef struct _DEVICE DEVICE;

typedef struct _FILE
{
    DEVICE* Device;
    BOOL    IsDevice;

    /* Followed by file-system specific data */
} FILE;

#define CACHE_SIZE  32  /* Remember this amount of sectors per device */

typedef struct _CACHE_ITEM
{
    ULONGLONG Tag;
    ULONG     Time;
    VOID*     Data;
} CACHE_ITEM;

typedef struct _CACHE
{
    ULONG nItems;
    ULONG Time;

    CACHE_ITEM Items[ CACHE_SIZE ];
} CACHE;

struct _DEVICE
{
    ULONG     DeviceId;
    ULONGLONG StartSector;
    ULONGLONG nSectors;
    BOOL      hasFileSystem;
    CACHE     Cache;

    /* File system information */
    FILE*     (*OpenFile)(DEVICE*, CHAR*);
    ULONGLONG (*ReadFile)(FILE*, VOID*, ULONGLONG);
    ULONGLONG (*GetFileSize)(FILE*);
    BOOL      (*SetFilePointer)(FILE*, LONGLONG, INT);
    ULONGLONG (*GetFilePointer)(FILE*);
    VOID      (*CloseFile)(FILE*);
    VOID      (*Release)(DEVICE*);
    VOID*     Data;      /* Private File System data */

    /* For the linked list */
    DEVICE* Next;
};

ULONGLONG ReadSector( DEVICE* Device, ULONGLONG Sector, ULONGLONG nSectors, VOID* Buffer );

VOID IoInitialize( ULONG device );

/* Returns TRUE if the Path indicates a device (instead of a file) */
BOOL IsDevice( FILE* File );

/* @From values for SeekFile */
#define FILE_BEGIN   0
#define FILE_CURRENT 1
#define FILE_END     2

FILE*     OpenFile      ( CHAR* Path );
ULONGLONG ReadFile      ( FILE* File, VOID* Buffer, ULONGLONG nBytes );
BOOL      SetFilePointer( FILE* File, LONGLONG Offset, INT From );
ULONGLONG GetFilePointer( FILE* File );
ULONGLONG GetFileSize   ( FILE* File );
VOID      CloseFile     ( FILE* File );

#endif
