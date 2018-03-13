#ifndef STDLIB_H
#define STDLIB_H

#include <types.h>

#define EXIT_SUCCESS  (0)
#define EXIT_FAILURE (-1)

/* String/Numeric functions */
ULONG strtoul(CHAR *s, CHAR **endptr, INT radix);

/* Heap functions */
VOID* malloc( ULONG nBytes );
VOID  free( VOID* Address );
VOID* realloc( VOID* Address, ULONG nBytes );

/* Non standard functions */
VOID  HeapInit( VOID* Address, ULONG Size );

/* Waits for an interrupt */
VOID WaitForInterrupt( VOID );
BOOL EnableA20Gate( VOID );
VOID CallAsBootsector( ULONG Drive, ULONG addr );

#endif
