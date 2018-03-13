#include <bios.h>
#include <ctype.h>
#include <video.h>

static UCHAR Attr = 7;

INT WriteChar( CHAR c )
{
    REGS regs;

    if (!iscntrl(c))
    {
        regs.h.ah = 9;
        regs.h.al = c;
        regs.h.bh = 0;
        regs.h.bl = Attr;
        regs.x.cx = 1;

        int86( 0x10, &regs, &regs );
    }

    regs.h.ah = 14;
    regs.h.al = c;
    regs.x.bx = 7;

    int86( 0x10, &regs, &regs );

    return c;
}

VOID ShowCursor( BOOL Show )
{
    REGS regs;

    regs.h.ah = 3;
    regs.h.bh = 0;
    int86( 0x10, &regs, &regs );

    regs.h.ah = 1;
    regs.h.ch = regs.h.ch & 0x1F;
    if (!Show)
    {
        regs.h.ch |= 0x20;
    }
    int86( 0x10, &regs, &regs );
}

VOID GotoXY( ULONG X, ULONG Y )
{
    REGS regs;

    regs.h.ah = 2;
    regs.h.bh = 0;
    regs.h.dl = X;
    regs.h.dh = Y;

    int86( 0x10, &regs, &regs );
}

VOID ClearScreen( VOID )
{
    REGS regs;

    regs.h.ah = 6;
    regs.h.al = 0;
    regs.h.bh = Attr;
    regs.x.cx = 0;
    regs.x.dx = 0x1850;
    int86( 0x10, &regs, &regs );

    GotoXY( 0, 0 );
}

VOID SetBkColor( UCHAR color )
{
    Attr = (Attr & 0x0F) | (color << 4);
}

VOID SetTextColor( UCHAR color )
{
    Attr = (Attr & 0xF0) | (color & 0x0F);
}

