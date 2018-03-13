#ifndef BIOS_H
#define BIOS_H

#include <types.h>

/* Register structure definitions for int86() */
typedef struct _DWORDREGS
{
    ULONG eax, ebx, ecx, edx, esi, edi, eflags;
    USHORT cflag, ds, es;
} DWORDREGS;

typedef struct _WORDREGS
{
    USHORT ax, __0, bx, __1, cx, __2, dx, __3, si, __4, di, __5, flags, __6;
    USHORT cflag, ds, es;
} WORDREGS;

typedef struct _BYTEREGS
{
    UCHAR al, ah; USHORT __0;
    UCHAR bl, bh; USHORT __1;
    UCHAR cl, ch; USHORT __2;
    UCHAR dl, dh; USHORT __3;
} BYTEREGS;

typedef union _REGS
{
    DWORDREGS d;
    WORDREGS  x;
    BYTEREGS  h;
} REGS;

VOID int86(UINT intno, REGS* inregs, REGS* outregs);

#endif

