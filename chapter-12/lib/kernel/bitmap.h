#ifndef __LIB_KERNEL_BITMAP_H__
#define __LIB_KERNEL_BITMAP_H__
#include "global.h"

#define BITMAP_MASK 1
// 在遍历位图时,整体上以字节为单位, 细节上以位为单位. 所以此处位图的指针必须是单字节
struct bitmap{
    uint32_t btmp_bytes_len;
    uint8_t* bits;
};

void bitmap_init(struct bitmap* bitmap);
bool bitmap_scan_test(struct bitmap* bitmap, uint32_t bit_idx);
int bitmap_scan(struct bitmap* btmp, uint32_t cnt);
void bitmap_set(struct bitmap* btmp, uint32_t bit_idx, int8_t vlaue);

#endif /* __LIB_KERNEL_BITMAP_H__ */

