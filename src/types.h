#ifndef TYPES_H
#define TYPES_H

#define NULL     ((VOID*)0)
#define CONST    const

#define TRUE     1
#define FALSE    0

#define PACKED   __attribute__ ((packed))

typedef void                VOID,   *PVOID;

typedef char                CHAR,   *PCHAR;
typedef short               SHORT,  *PSHORT;
typedef int                 INT,    *PINT;
typedef long                LONG,   *PLONG;
typedef long long           LONGLONG;

typedef unsigned char       UCHAR,  *PUCHAR;
typedef unsigned short      USHORT, *PUSHORT;
typedef unsigned int        UINT,   *PUINT;
typedef unsigned long       ULONG,  *PULONG;
typedef unsigned long long  ULONGLONG;

typedef USHORT              WCHAR,  *PWCHAR;
typedef UINT                BOOL,   *PBOOL;

#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#define MIN(a,b) (((a) < (b)) ? (a) : (b))

#define LOWORD(x) ((x) & 0xFFFF)
#define HIWORD(x) (((x) >> 16) & 0xFFFF)
#define LOBYTE(x) ((x) & 0xFF)
#define HIBYTE(x) (((x) >> 8) & 0xFF)

#define MAKELONG(hi,lo) (((hi) << 16) | (lo))

/* Some defines for working with 16-bit pointers */
#define SEG( p )    (((ULONG)(p) >> 4) & 0xFFFF)
#define OFS( p )    (((ULONG)(p)) & 0x000F)
#define PTR( s, o ) ((VOID*)(((s) << 4) + (o)))

#endif
