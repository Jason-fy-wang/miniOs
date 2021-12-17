#ifndef __LIB_KERNEL_PRINT_H__
#define __LIB_KERNEL_PRINT_H__

#include "stdint.h"
void put_char(const uint8_t c);
void put_str(const char *message);
void put_int(const uint32_t num);
#endif