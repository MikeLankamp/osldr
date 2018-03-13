#ifndef VIDEO_H
#define VIDEO_H

#include <types.h>

#define COLOR_BLACK         0
#define COLOR_BLUE          1
#define COLOR_GREEN         2
#define COLOR_CYAN          3
#define COLOR_RED           4
#define COLOR_MAGENTA       5
#define COLOR_BROWN         6
#define COLOR_LIGHTGRAY     7
#define COLOR_DARKGRAY      8
#define COLOR_LIGHTBLUE     9
#define COLOR_LIGHTGREEN   10
#define COLOR_LIGHTCYAN    11
#define COLOR_LIGHTRED     12
#define COLOR_LIGHTMAGENTA 13
#define COLOR_YELLOW       14
#define COLOR_WHITE        15

INT  WriteChar( CHAR c );
VOID ShowCursor( BOOL Show );
VOID GotoXY( ULONG X, ULONG Y );
VOID ClearScreen( VOID );
VOID SetBkColor( UCHAR color );
VOID SetTextColor( UCHAR color );

#endif
