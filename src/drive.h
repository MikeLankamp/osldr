#ifndef DRIVE_H
#define DRIVE_H

#include <types.h>

typedef struct _DRIVE_INFO
{
    USHORT    Drive;           /* 0 - 255 are valid */
    USHORT    EddVersion;
    USHORT    ControllerFlags;
    USHORT    DriveFlags;
    ULONG     nCylinders;
    ULONG     nHeads;
    ULONG     nSectors;
    ULONGLONG nTotalSectors;
    USHORT    nBytesPerSector;
} PACKED DRIVE_INFO;

/*
 * Resets the drive system.
 * Bit 7 of @Drive must be set when querying HDDs (BIOS Convention).
 * TRUE is returned on success.
 */
BOOL ResetDrive( UCHAR Drive );

/*
 * Returns the drive parameters of @Drive.
 * Bit 7 of @Drive must be set when querying HDDs (BIOS Convention).
 * NULL is returned on failure. The returned pointer points to a static
 * buffer and should not be used other then to read from and may be
 * invalid after a next call.
 */
DRIVE_INFO* GetDriveParameters( UCHAR Drive );

/*
 * Reads several sectors from a drive into Buffer.
 * Bit 7 of @Drive must be set when reading HDDs (BIOS Convention).
 * The number of read sectors is returned.
 */
ULONGLONG ReadDrive( UCHAR Drive, ULONGLONG Sector, ULONGLONG nSectors, VOID* Buffer );

#endif
