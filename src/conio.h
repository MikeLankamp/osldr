#ifndef CONIO_H
#define CONIO_H

#include <types.h>

/* Some virtual key scan codes as returned by getch() */
#define VK_ESC      0x01
#define VK_1        0x02
#define VK_2        0x03
#define VK_3        0x04
#define VK_4        0x05
#define VK_5        0x06
#define VK_6        0x07
#define VK_7        0x08
#define VK_8        0x09
#define VK_9        0x0A
#define VK_0        0x0B

#define VK_F1       0x3B
#define VK_F2       0x3C
#define VK_F3       0x3D
#define VK_F4       0x3E
#define VK_F5       0x3F
#define VK_F6       0x40
#define VK_F7       0x41
#define VK_F8       0x42
#define VK_F9       0x43
#define VK_F10      0x44
#define VK_F11      0x57
#define VK_F12      0x58

#define VK_ENTER    0x1C
#define VK_HOME     0x47
#define VK_UP       0x48
#define VK_PREV     0x49
#define VK_LEFT     0x4B
#define VK_RIGHT    0x4D
#define VK_END      0x4F
#define VK_DOWN     0x50
#define VK_NEXT     0x51
#define VK_INSERT   0x52
#define VK_DELETE   0x53

INT kbhit( VOID );
INT getch( VOID );

#endif
