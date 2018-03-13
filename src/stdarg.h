#ifndef STDARG_H
#define STDARG_H

#include <types.h>

typedef CHAR* va_list;

#define __size(x) ((sizeof(x) + sizeof(INT) - 1) & -sizeof(INT))

#define va_copy(dest, src)  ((VOID)((dest) = (src)))
#define va_start(ap, parmN) ((VOID)((ap) = (va_list)( (CHAR*)(&(parmN)) + __size(parmN)) ))
#define va_arg(ap, type)    (*(type *)( ((ap) += __size(type)) - __size(type) ))
#define va_end(ap)          ((VOID)0)

#endif
