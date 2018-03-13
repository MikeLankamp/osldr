#ifndef STRING_H
#define STRING_H

#include <types.h>

VOID* memset ( VOID* Src,  INT c,     INT n );
VOID* memchr ( VOID* Src,  INT c,     INT n );
VOID* memcpy ( VOID* Dest, VOID* Src, INT n );
VOID* memmove( VOID* Dest, VOID* Src, INT n );

CHAR* strchr ( CONST CHAR* Src,  INT c );
CHAR* strcpy ( CHAR* Dest, CONST CHAR* Src );
INT   stricmp( CONST CHAR* s1,   CONST CHAR* s2 );
INT   strncmp( CONST CHAR* s1,   CONST CHAR* s2,  INT n );
CHAR* strncpy( CHAR* Dest, CONST CHAR* Src, INT n );
INT   strlen ( CONST CHAR* Src );
CHAR* strupr ( CHAR* Src );

WCHAR* wcschr ( CONST WCHAR* Src,  INT c );
WCHAR* wcscpy ( WCHAR* Dest, CONST WCHAR* Src );
WCHAR* wcsicmp( WCHAR* Dest, CONST WCHAR* Src );
WCHAR* wcsupr ( WCHAR* Src );
INT    wcslen ( CONST WCHAR* Src );

#endif
