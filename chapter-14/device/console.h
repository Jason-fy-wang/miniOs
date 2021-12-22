#ifndef _DEVICE_CONSOLE_H_
#define _DEVICE_CONSOLE_H_
#include "stdint.h"


void console_put_int(const uint32_t num);
void console_put_char(const uint8_t c);
void console_put_str(const char* str);
void console_release(void);
void console_acquire(void);
void console_init(void);


#endif // _DEVICE_CONSOLE_H_