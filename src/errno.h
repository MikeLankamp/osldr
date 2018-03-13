#ifndef ERRNO_H
#define ERRNO_H

#include <types.h>

#define EZERO       0   /* No error */
#define ENOMEM      1   /* Not enough memory */
#define ENOENT      2   /* File not found */
#define ERANGE      3   /* Value too big */
#define EFAULT      4   /* Unknown error */
#define ENODEV      5   /* Device not found */
#define ENOPART     6   /* Partition not found */
#define ENOFSYS     7   /* Unknown filesystem */
#define EIO         8   /* I/O Error */
#define ECORRUPT    9   /* File is corrupt */
#define EFTYPE     10   /* Unknown file type */

extern INT errno;

#endif
