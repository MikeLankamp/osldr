#ifndef STDIO_H
#define STDIO_H

#include <stdarg.h>

#define EOF (-1)                /* End of file indicator */

INT putchar( INT c );
INT puts( CONST CHAR* s );

INT printf   ( CONST CHAR* fmt,                     ... );
INT snprintf ( CHAR* s,   ULONG n, CONST CHAR* fmt, ... );
INT sprintf  ( CHAR* s,            CONST CHAR* fmt, ... );
INT vprintf  ( CONST CHAR* fmt,                     va_list arg );
INT vsnprintf( CHAR* s,   ULONG n, CONST CHAR* fmt, va_list arg );
INT vsprintf ( CHAR* s,            CONST CHAR* fmt, va_list arg );

#endif
