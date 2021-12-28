#include "assert.h"
#include "stdio.h"


void user_spin(char* filename, int line, const char* func, const char* consition){
    printf("\nfilename: %s \n line %d \n function %s \n condition %s \n", filename, line, func, consition);
    while(1);
}
