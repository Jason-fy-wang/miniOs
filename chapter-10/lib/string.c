#include "string.h"
#include "debug.h"
#include "global.h"

void memset(void *dst_, uint8_t value, uint32_t size) {
    ASSERT(dst_ != NULL);
    uint8_t* dst = (uint8_t *)dst_;
    while(size-- > 0) {
        *dst++ = value;
    }
}

void memcpy(void *dst_, const void* src_,uint32_t size) {
    ASSERT(dst_ != NULL && src_ != NULL);
    uint8_t* dst = (uint8_t *)dst_;
    const uint8_t* src = (const uint8_t *)src;

    while(size-- > 0){
        *dst++ = *src++;
    }
}

// 连续比较以地址a和地址b开头的size个字节,若相等则返回0. a>b,则返回1, 否则返回-1
int memcmp(const void *a_, const void* b_,uint32_t size){
    const char *a = (const char *)a_;
    const char *b = (const char *)b_;

    ASSERT(a != NULL && b != NULL);
    while(size -- > 0){
        if((*a) != (*b)){
            return (*a) > (*b) ? 1:-1;
        }
        a++;
        b++;
    }
    return 0;
}

// 将字符串从src拷贝到 dst
char* strcpy(char *dst_, const char* src_){
    ASSERT(dst_ != NULL && src_ != NULL);

    char * r  = dst_;   // 用于返回字符串新位置
    while((*dst_++ = *src_++));
    return r;
}

// 字符串长度
uint32_t strlen(const char *str){
    ASSERT(str != NULL);

    const char* p = str;
    while(*p++);
    return (p-str-1);
}

int8_t strcmp(const char* a, const char* b){
    ASSERT(a != NULL && b != NULL);

    while((*a) != 0 && (*a) == (*b)){
        a++;
        b++;
    }
    return (*a) > (*b) ? 1:((*a) < (*b) ? -1:0);
}

char * strchr(const char* str, const uint8_t ch){
    ASSERT(str != NULL);
    while((*str) != 0){
        if((*str) == ch){
            return (char*) str;
        }
        str++;
    }
    return NULL;
}


char* strrchr(const char* str, const uint8_t ch){
    ASSERT(str != NULL);
    const char* last_char = NULL;

    while((*str) != 0){
        if((*str) == ch){
            last_char = str;
        }
        str++;
    }
    return (char*) last_char;
}



char * strcat(char* dst_, const char* src_){
    ASSERT(dst_ != NULL && src_ != NULL);
    char *str = dst_;
    while(*str++);
    --str;
    while((*str++ = *src_++));
    return dst_;
}

// 在字符串str中查找 ch出现的次数
uint32_t strchrs(const char* str, uint8_t ch){
    ASSERT(str != NULL);

    uint32_t ch_cnt = 0;
    const char* p = str;
    while(*p != 0){
        if(*p == ch){
            ch_cnt++;
        }
        p++;
    }
    return ch_cnt;
}


