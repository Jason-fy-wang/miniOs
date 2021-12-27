#include "print.h"
#include "init.h"
#include "debug.h"
#include "thread.h"
#include "interrupt.h"
#include "console.h"
#include "process.h"
#include "syscall_init.h"
#include "syscall.h"
#include "stdio.h"
#include "fs.h"
#include "dir.h"

void k_thread_a(void *);
void k_thread_b(void*);
void u_prog_a(void);
void u_prog_b(void);

int prog_a_pid=0, prog_b_pid=0;

int main(void){
    put_str("i am kernel\n");
    init_all();

    //process_execute(u_prog_a, "user_a");
    //process_execute(u_prog_b, "user_b");

    //打开中断
    intr_enable();
    //thread_start("threadA", 31, k_thread_a,"A_");
    //thread_start("threadB", 31, k_thread_b,"B_");

    // uint32_t fd = sys_open("/file1", O_CREAT|O_RDWR);
    // printf("fd: %d\n", fd);
    // sys_write(fd, "hello world\n", 12);
    // sys_close(fd);
    //printf( "/dir1/subdir1 create %s\n", sys_mkdir("/dir1/subdir1")==0?"done":"failed");

    // printf("/dir1 create %s\n", sys_mkdir("/dir1")==0?"done":"failed");

    // printf( "new /dir1/subdir1 create %s\n", sys_mkdir("/dir1/subdir1")==0?"done":"failed");

    // uint32_t fd1 = sys_open("/dir1/subdir1/file2", O_RDWR);
    // if(fd1 != -1){
    //     printf("/dir1/subdir1/file2 create done.\n");
    //     sys_write(fd1,"catch me if you can.\n", 21);
    //     sys_lseek(fd1, 0, SEEK_SET);
    //     char buf[32] = {0};
    //     sys_read(fd1, buf, 21);
    //     printf("/dir1/subdir1/file2 says: %s\n", buf);
    //     sys_close(fd1);
    // }else {
    //     printf("/dir1/subdir1/file2  open failed.\n");
    // }

    struct dir* p_dir = sys_opendir("/dir1/subdir1");
    if(p_dir){
        printf("/dir1/subdir1 open done.\n");
        char* type = NULL;
        struct dir_entry* dir_e = NULL;
        while((dir_e = sys_readdir(p_dir))){
            if(dir_e->f_type == FT_REGULAR){
                type = "regular";
            }else {
                type = "dictory";
            }
            printf("  %s %s \n", type, dir_e->filename);
        }

        printf("try to delete nonempty directory /dir1/subdir1.\n");

        if(sys_rmdir("/dir1/subdir1") == -1){
            printf("sys_rmdir: /dir1/subdir1 delete failed.\n");
        }

        printf("try to delete /dir1/subdir1/file2 .\n");

        if(sys_rmdir("/dir1/subdir1/file2") == -1){
            printf("sys_rmdir: /dir1/subdir1/file2 delete failed.\n");
        }

        if(sys_unlink("/dir1/subdir1/file2") == 0){
            printf("sys_unlink: /dir1/subdir1/file2 delete done.\n");
        }

        printf("try to delete directory /dir1/subdir1 again.\n");

        if(sys_rmdir("/dir1/subdir1") == 0){
            printf("/dir1/subdir1 delete done.\n");
        }

        printf("/dir1 content after delete /dir1/subdir1: \n");
        sys_rewinddir(p_dir);

        while((dir_e = sys_readdir(p_dir))){
            if(dir_e->f_type == FT_REGULAR){
                type = "regular";
            }else {
                type = "dictory";
            }
            printf("  %s %s \n", type, dir_e->filename);
        }

        if(sys_closedir(p_dir) == 0){
            printf("/dir1/subdir1 close done.\n");
        }else {
            printf("/dir1/subdir1 close failed.\n");
        }

    }else {
        printf("/dir1/subdir1 open failed.\n");
    }

    while (1);
    return 0;
}

void k_thread_a(void *arg_){
    char * arg = arg_;
    void* addr1 = sys_malloc(256);
    void* addr2 = sys_malloc(255);
    void* addr3 = sys_malloc(254);
    console_put_str(" thread_a malloc addr:0x");
    console_put_int((int)addr1);
    console_put_char(',');
    console_put_int((int)addr2);
    console_put_char(',');
    console_put_int((int)addr3);
    console_put_char('\n');
    int cpu_delay = 100000;
    while(cpu_delay-- >0);
    sys_free(addr1);
    sys_free(addr2);
    sys_free(addr3);
    console_put_str(" thread_a end\n");
    while(1);
}


void k_thread_b(void* arg_){
    char *arg = arg_;
       void* addr1 = sys_malloc(256);
    void* addr2 = sys_malloc(255);
    void* addr3 = sys_malloc(254);
    console_put_str(" thread_b malloc addr:0x");
    console_put_int((int)addr1);
    console_put_char(',');
    console_put_int((int)addr2);
    console_put_char(',');
    console_put_int((int)addr3);
    console_put_char('\n');
    int cpu_delay = 100000;
    while(cpu_delay-- >0);
    sys_free(addr1);
    sys_free(addr2);
    sys_free(addr3);
    console_put_str(" thread_b end\n");
    while(1);
}



void u_prog_a(void){
    char* name = "prog_a";
    printf(" I am %s, my pid is: 0x%x%c", name, get_pid(),'\n');
    void* addr1 = malloc(256);
    void* addr2 = malloc(255);
    void* addr3 = malloc(254);
    printf( "prog_a malloc addr: 0x%x, 0x%x, 0x%x\n", (int)addr1,(int)addr2,(int)addr3);
    int cpu_delay = 100000;
    while(cpu_delay-- >0);
    free(addr1);
    free(addr2);
    free(addr3);
    while(1);
}
void u_prog_b(void){
    char* name = "prog_b";
    printf(" I am %s, my pid is: 0x%x%c", name, get_pid(),'\n');
       void* addr1 = malloc(256);
    void* addr2 = malloc(255);
    void* addr3 = malloc(254);
    printf( "prog_b malloc addr: 0x%x, 0x%x, 0x%x\n", (int)addr1,(int)addr2,(int)addr3);
    int cpu_delay = 100000;
    while(cpu_delay-- >0);
    free(addr1);
    free(addr2);
    free(addr3);
    while(1);
}



void test_thread_a(){
        void* addr1;
    void* addr2;
    void* addr3;
    void* addr4;
    void* addr5;
    void* addr6;
    void* addr7;
    console_put_str("  thread_a_start\n");
    int max = 1000;
    while(max-- > 0){
        int size  = 128;
        addr1 = sys_malloc(size);
        size *= 2;
        addr2 = sys_malloc(size);
        size *= 2;
        addr3 = sys_malloc(size);
        sys_free(addr1);
        addr4 = sys_malloc(size);
        size *= 2;
        size *= 2;
        size *= 2;
        size *= 2;
        size *= 2;
        size *= 2;
        size *= 2;
        addr5 = sys_malloc(size);
        addr6 = sys_malloc(size);
        sys_free(addr5);
        size *= 2;

        addr7 = sys_malloc(size);
        console_put_str("addr7=");
        console_put_int((int)addr7);
        sys_free(addr6);
        sys_free(addr7);
        sys_free(addr2);
        sys_free(addr3);
        sys_free(addr4);
    }
}


void test_thread_b(){
     void* addr1;
    void* addr2;
    void* addr3;
    void* addr4;
    void* addr5;
    void* addr6;
    void* addr7;
    void* addr8;
    void* addr9;
    console_put_str("  thread_b_start\n");
    int max = 1000;
    while(max-- > 0){
        int size  = 9;
        addr1 = sys_malloc(size);
        size *= 2;
        addr2 = sys_malloc(size);
        size *= 2;
        sys_free(addr2);
        addr3 = sys_malloc(size);
        sys_free(addr1);
        addr4 = sys_malloc(size);
        addr5 = sys_malloc(size);
        addr6 = sys_malloc(size);
        sys_free(addr5);
        size *= 2;
        addr7 = sys_malloc(size);
        sys_free(addr6);
        sys_free(addr7);
        sys_free(addr3);
        sys_free(addr4);
        size *= 2;
        size *= 2;
        size *= 2;
        addr1 = sys_malloc(size);
        addr2 = sys_malloc(size);
        addr3 = sys_malloc(size);
        addr4 = sys_malloc(size);
        addr5 = sys_malloc(size);
        addr6 = sys_malloc(size);
        addr7 = sys_malloc(size);
        addr8 = sys_malloc(size);
        addr9 = sys_malloc(size);
        console_put_str("addr9=");
        console_put_int((int)addr9);
        sys_free(addr1);
        sys_free(addr2);
        sys_free(addr3);
        sys_free(addr4);
        sys_free(addr5);
        sys_free(addr6);
        sys_free(addr7);
        sys_free(addr8);
        sys_free(addr9);
    }
}
