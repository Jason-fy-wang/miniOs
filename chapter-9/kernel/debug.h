#ifndef _KERNEL_DEBUG_H_
#define _KERNEL_DEBUG_H_

void panic_spin(char *filename, int line, const char* func, const char* condition);

#define PANIC(...) panic_spin(__FILE__, __LINE__, __FUNCTION__,__VA_ARGS__)

#ifdef NDEBUG
    #define ASSERT(CONSITION) (void(0))
#else
    #define ASSERT(CONSITION) \
    if(CONSITION){}else {PANIC(#CONSITION);}
#endif /*NDEBUG*/

#endif /*_KERNEL_DEBUG_H_*/

