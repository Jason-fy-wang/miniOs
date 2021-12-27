#ifndef _LIB_STDIO_H_
#define _LIB_STDIO_H_
#include "stdint.h"
#include "global.h"

#define va_start(ap, v)  ap=(va_list)&v     // 把ap指向第一个固定参数v
#define va_arg(ap, t)    *((t*)(ap += 4))      // ap指向下一个参数并返回其值
#define va_end(ap)       ap = NULL
typedef char* va_list;

uint32_t printf(const char* format, ...);
uint32_t vsprintf(char* str, const char* format, va_list ap);
uint32_t sprintf(char* buf, const char*format, ...);
#endif /* _LIB_STDIO_H_ */