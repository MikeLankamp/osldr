#ifndef CTYPE_H
#define CTYPE_H

#include <types.h>

extern UCHAR _ctype[ 256 ];

/* Character classes */
#define _IS_UPP    0x0001           /* upper case */
#define _IS_LOW    0x0002           /* lower case */
#define _IS_DIG    0x0004           /* digit */
#define _IS_SP     0x0008           /* space */
#define _IS_PUN    0x0010           /* punctuation */
#define _IS_CTL    0x0020           /* control */
#define _IS_BLK    0x0040           /* blank */
#define _IS_HEX    0x0080           /* [0..9] or [A-F] or [a-f] */
#define _IS_ALPHA  (_IS_LOW | _IS_UPP)
#define _IS_ALNUM  (_IS_DIG | _IS_ALPHA)
#define _IS_GRAPH  (_IS_ALNUM | _IS_HEX | _IS_PUN)

#define isalnum(c)   (_ctype[ (UCHAR)(c) ] & (_IS_ALNUM))
#define isalpha(c)   (_ctype[ (UCHAR)(c) ] & (_IS_ALPHA))
#define isblank(c)   (_ctype[ (UCHAR)(c) ] & (_IS_BLK))
#define iscntrl(c)   (_ctype[ (UCHAR)(c) ] & (_IS_CTL))
#define isdigit(c)   (_ctype[ (UCHAR)(c) ] & (_IS_DIG))
#define isgraph(c)   (_ctype[ (UCHAR)(c) ] & (_IS_GRAPH))
#define islower(c)   (_ctype[ (UCHAR)(c) ] & (_IS_LOW))
#define isprint(c)   (_ctype[ (UCHAR)(c) ] & (_IS_GRAPH | _IS_BLK))
#define ispunct(c)   (_ctype[ (UCHAR)(c) ] & (_IS_PUN))
#define isspace(c)   (_ctype[ (UCHAR)(c) ] & (_IS_SP))
#define isupper(c)   (_ctype[ (UCHAR)(c) ] & (_IS_UPP))
#define isxdigit(c)  (_ctype[ (UCHAR)(c) ] & (_IS_HEX))

#define tolower(c)  ((isupper(c)) ? (c) - 'A' + 'a' : (c))
#define toupper(c)  ((islower(c)) ? (c) - 'a' + 'A' : (c))

#endif
