#include "debug.h"
#include "print.h"
#include "interrupt.h"

void panic_spin(char *filename, int line, const char* func, const char* condition){
    intr_disable();

    put_str("\n Error!!!!\n");
    put_str("fileName: ");put_str(filename);
    put_str(" line: 0x");put_int(line);
    put_str("  function: ");put_str(func);
    put_str(" condition: ");put_str(condition);
    while(1);
}


