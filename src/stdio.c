#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "video.h"

static VOID ulltoa(ULONGLONG value, CHAR* string, INT base)
{
    CHAR* end = string;
    do
    {
        *end = (value % base) + '0';
        if (*end > '9')
        {
            *end += 'a' - '9' - 1;
        }
        value = value / base;
        end++;
    } while (value > 0);
    *end-- = '\0';

    /* Reverse string */
    while (string < end)
    {
        CHAR c    = *end;
        *end--    = *string;
        *string++ = c;
    }
}


#define PRINT(c)                 do { if (!useString) putchar(c); else if (n > 0) *s++ = (c); n--; } while(0)

#define FLAG_FORCE_SIGN          1
#define FLAG_LEFT_JUSTIFY        2
#define FLAG_BLANK_SIGN          4
#define FLAG_ALTERNATE_FORM      8
#define FLAG_ZERO_PAD           16

static INT _printf_execute( va_list *va, CHAR* s, BOOL useString, INT n, INT Flags, INT Width, INT Length, CHAR Type )
{
    INT      start = n;
    CHAR     buf[32];
    CHAR*    src  = buf;
    WCHAR*   wsrc = (WCHAR*)buf;
    LONGLONG sign = 0;
    INT      i;

    switch (Type)
    {
        case 'i':
        case 'd':
        {
            switch (Length)
            {
                case 1:  sign = va_arg(*va, CHAR); break;
                case 2:  sign = va_arg(*va, SHORT); break;
                case 4:  sign = va_arg(*va, LONG); break;
                case 8:  sign = va_arg(*va, LONGLONG); break;
                default: sign = va_arg(*va, INT); break;
            }

            /* No alternate form exists */
            Flags &= ~FLAG_ALTERNATE_FORM;

            if (sign < 0)
            {
                /* Ignore '-', but force sign */
                Flags |= FLAG_FORCE_SIGN;
                ulltoa( -sign, buf, 10 );
            }
            else
            {
                ulltoa( sign, buf, 10 );
            }

            Length = 1;
            break;
        }

        case 'u':
            /* No alternate form exists */
            Flags &= ~(FLAG_ALTERNATE_FORM);
        case 'x':
        case 'X':
        {
            ULONGLONG value;

            switch (Length)
            {
                case 1:  value = va_arg(*va, UCHAR); break;
                case 2:  value = va_arg(*va, USHORT); break;
                case 4:  value = va_arg(*va, ULONG); break;
                case 8:  value = va_arg(*va, ULONGLONG); break;
                default: value = va_arg(*va, UINT); break;
            }

            ulltoa( value, buf, (Type == 'u') ? 10 : 16 );

            /* Not a signed conversion */
            Flags &= ~(FLAG_FORCE_SIGN | FLAG_BLANK_SIGN);

            if (value > 0)
            {
                sign = 1;
            }

            if (Type == 'X')
            {
                strupr( buf );
            }

            Length = 1;
            break;
        }

        case 's':
            Flags &= ~(FLAG_FORCE_SIGN | FLAG_BLANK_SIGN | FLAG_ALTERNATE_FORM | FLAG_ZERO_PAD);
            if (Length < 4)
            {
                src = va_arg(*va, CHAR*);
                Length = 1;
            }
            else
            {
                src = (CHAR*)va_arg(*va, WCHAR* );
                Length = 2;
            }

            if (src == NULL)
            {
                src = "(null)";
                Length = 1;
            }
            break;

        case 'c':
            Flags &= ~(FLAG_FORCE_SIGN | FLAG_BLANK_SIGN | FLAG_ALTERNATE_FORM | FLAG_ZERO_PAD);
            if (Length == 0)
            {
                src[0] = va_arg(*va, CHAR);
                src[1] = '\0';
                Length = 1;
            }
            else
            {
                wsrc[0] = va_arg(*va, WCHAR);
                wsrc[1] = L'\0';
                Length = 2;
            }
            break;

        case 'p':
        {
            PVOID p = va_arg(*va, PVOID);
            Flags &= ~(FLAG_FORCE_SIGN | FLAG_BLANK_SIGN | FLAG_ALTERNATE_FORM | FLAG_ZERO_PAD);
            sprintf( buf, "%08X", p );
            Length = 1;
            break;
        }
    }

    /* Adjust padding width */
    if (Flags & FLAG_ALTERNATE_FORM)
    {
        if (sign != 0)
        {
            Width -= 2;
        }
    }
    else if (Flags & FLAG_FORCE_SIGN)
    {
        Width--;
    }
    else if (Flags & FLAG_BLANK_SIGN)
    {
        Width--;
    }

    /* Print sign space in front of padding */
    if ((Flags & FLAG_ZERO_PAD) || (Flags & FLAG_LEFT_JUSTIFY))
    {
        if (Flags & FLAG_ALTERNATE_FORM)
        {
            /* Type is 'x' or 'X' */
            if (sign != 0)
            {
                PRINT('0');
                PRINT( Type );
            }
        }
        else if (Flags & FLAG_FORCE_SIGN)
        {
            PRINT( (sign < 0) ? '-' : '+' );
        }
        else if (Flags & FLAG_BLANK_SIGN)
        {
            PRINT( (sign < 0) ? '-' : ' ' );
        }
    }

    /* Print value first when left justified */
    if (Flags & FLAG_LEFT_JUSTIFY)
    {
        /* No zero padding when left aligned */
        Flags &= ~FLAG_ZERO_PAD;

        for (i = 0; src[i] != '\0'; i += Length, s += Length-1 ) PRINT( src[i] );
    }

    /* Print padding */
    for (i = 0; i < Width - strlen(src); i++)
    {
        PRINT( (Flags & FLAG_ZERO_PAD) ? '0' : ' ');
    }

    /* Print sign space after padding */
    if (~Flags & FLAG_LEFT_JUSTIFY)
    {
        if (~Flags & FLAG_ZERO_PAD)
        {
            if (Flags & FLAG_ALTERNATE_FORM)
            {
                if (sign != 0)
                {
                    PRINT('0');
                    PRINT( Type );
                }
            }
            else if (Flags & FLAG_FORCE_SIGN)
            {
                PRINT( (sign < 0) ? '-' : '+' );
            }
            else if (Flags & FLAG_BLANK_SIGN)
            {
                PRINT( (sign < 0) ? '-' : ' ' );
            }
        }

        /* Print value after padding when right justified */
        for (i = 0; src[i] != '\0'; i += Length, s += Length-1 ) PRINT( src[i] );
    }

    return (start - n);
}

INT _printf(CHAR* s, BOOL useString, INT n, CONST CHAR* format, va_list arg)
{
    INT start = n;

    for(; *format != '\0'; format++)
    {
        if (*format == '%')
        {
            format++;
            if (*format != '%')
            {
                INT  Flags     =  0;
                INT  Width     =  0;
                INT  Length    =  0;
                BOOL abort;

                /* Read flag character(s) */
                for (abort = FALSE; !abort; format++)
                {
                    switch (*format)
                    {
                        case '+': Flags |= FLAG_FORCE_SIGN;     break;
                        case '-': Flags |= FLAG_LEFT_JUSTIFY;   break;
                        case ' ': Flags |= FLAG_BLANK_SIGN;     break;
                        case '#': Flags |= FLAG_ALTERNATE_FORM; break;
                        case '0': Flags |= FLAG_ZERO_PAD;       break;
                        default:  abort = TRUE; format--;       break;
                    }
                }

                /* Read width specifier */
                if (*format == '*')
                {
                    Width = va_arg(arg, INT);
                    format++;
                }
                else
                {
                    while (isdigit(*format))
                    {
                        Width = (Width * 10) + (*format++ - '0');
                    }
                }

                /* Read length modifier */
                switch (*format)
                {
                    case 'h':
                        Length = 2;
                        if (*++format == 'h')
                        {
                            Length = 1;
                            format++;
                        }
                        break;

                    case 'l':
                        Length = 4;
                        if (*++format == 'l')
                        {
                            Length = 8;
                            format++;
                        }
                        break;
                }

                /* Read conversion specifier */
                switch (*format)
                {
                    case 'd': case 'i': case 'x':
                    case 'X': case 'o': case 'u':
                    case 'c': case 's': case 'p':
                    {
                        INT len = _printf_execute( &arg, s, useString, n, Flags, Width, Length, *format );
                        n -= len;
                        s += len;
                        break;
                    }
                }

                continue;
            }
        }

        /* Add character */
        PRINT( *format );
    }

    if (useString)
    {
        PRINT('\0');
    }

    return start - n;
}

#undef PRINT

INT putchar(INT c)
{
    if (c == '\n')
    {                 
        putchar('\r');
    }

    return WriteChar( c );
}

INT puts(CONST CHAR* s)
{
    while (*s != '\0')
    {
        if (putchar(*s++) == EOF)
        {
            return EOF;
        }
    }
    putchar('\n');
    return 1;
}

INT printf(CONST CHAR* format, ...)
{
    INT     rv;
    va_list arg;

    va_start( arg, format );
    rv = vprintf( format, arg );
    va_end( arg );

    return rv;
}

INT snprintf(CHAR* s, ULONG n, CONST CHAR* format, ...)
{
    va_list arg;
    INT     rv;

    va_start( arg, format );
    rv = vsnprintf( s, n, format, arg );
    va_end( arg );

    return rv;
}

INT sprintf(CHAR* s, CONST CHAR* format, ...)
{
    va_list arg;
    INT     rv;

    va_start( arg, format );
    rv = vsprintf( s, format, arg );
    va_end( arg );

    return rv;
}

INT vprintf(CONST CHAR* format, va_list arg)
{
    return _printf( NULL, FALSE, 0, format, arg );
}

INT vsprintf(CHAR* s, CONST CHAR* format, va_list arg)
{
    return _printf( s, TRUE, INT_MAX, format, arg );
}

INT vsnprintf(CHAR* s, ULONG n, CONST CHAR* format, va_list arg)
{
    return _printf( s, TRUE, n, format, arg );
}

