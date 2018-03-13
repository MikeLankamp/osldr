#include <bios.h>
#include <types.h>

INT kbhit( VOID )
{
    REGS regs;

    regs.h.ah = 0x11;
    int86( 0x16, &regs, &regs );

    /* Return inverted Zero Flag */
    return ~regs.x.flags & 0x40;
}

INT getch( VOID )
{
    REGS regs;

    regs.h.ah = 0x10;
    int86( 0x16, &regs, &regs );

    /* Return scan code (!) */
    return regs.h.ah;
}
