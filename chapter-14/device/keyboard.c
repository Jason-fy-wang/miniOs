#include "keyboard.h"
#include "print.h"
#include "interrupt.h"
#include "io.h"
#include "global.h"
#include "ioqueue.h"

#define KBD_BUF_PORT 0x60   //键盘buffer寄存器端口号

// 转义字符定义部分控制字符(转义或acii 码值)
#define ESC '\033'
#define BACKSPACE   '\b'
#define TAB     '\t'
#define ENTER   '\r'
#define DELETE   '\177'

#define CHAR_INVISIBLE  0
#define CTRL_L_CHAR     CHAR_INVISIBLE
#define CTRL_R_CHAR     CHAR_INVISIBLE
#define SHIFT_L_CHAR    CHAR_INVISIBLE
#define SHIFT_R_CHAR    CHAR_INVISIBLE
#define ALT_L_CHAR      CHAR_INVISIBLE
#define ALT_R_CHAR      CHAR_INVISIBLE
#define CAPS_LOCK_CHAR  CHAR_INVISIBLE

//控制字符的通码和断码
#define SHIFT_L_MAKE    0x2a
#define SHIFT_R_MAKE    0x36
#define ALT_L_MAKE      0x38
#define ALT_R_MAKE      0xE038
#define ALT_R_BREAK     0xe0b8
#define CTRL_L_MAKE     0x1d
#define CTRL_R_MAKE     0xe01d
#define CTRL_R_BREAK    0xe09d
#define CAPS_LOCK_MAKE  0x3a

// 定义变量用于记录相应按键是否按下
static bool ctrl_status, shift_status,alt_status,caps_lock_status, ext_scancode;

struct ioqueue kbd_buf;

// 通码make_code为索引的二维数组
static char keymap[][2]= {
/*0x00*/    {0,0},
/*0x01*/    {ESC,ESC},
/*0x02*/    {'1','!'},
/*0x03*/    {'2','@'},
/*0x04*/    {'3','#'},
/*0x05*/    {'4','$'},
/*0x06*/    {'5','%'},
/*0x07*/    {'6','^'},
/*0x08*/    {'7','&'},
/*0x09*/    {'8','*'},
/*0x0A*/    {'9','('},
/*0x0B*/    {'0',')'},
/*0x0C*/    {'-','_'},
/*0x0D*/    {'=','+'},
/*0x0E*/    {BACKSPACE,BACKSPACE},
/*0x0F*/    {TAB,TAB},
/*0x10*/    {'q','Q'},
/*0x11*/    {'w','W'},
/*0x12*/    {'e','E'},
/*0x13*/    {'r','R'},
/*0x14*/    {'t','T'},
/*0x15*/    {'y','Y'},
/*0x16*/    {'u','U'},
/*0x17*/    {'i','I'},
/*0x18*/    {'o','O'},
/*0x19*/    {'p','P'},
/*0x1a*/    {'[','{'},
/*0x1b*/    {']','}'},
/*0x1c*/    {ENTER,ENTER},
/*0x1d*/    {CTRL_L_CHAR,CTRL_L_CHAR},
/*0x1e*/    {'a','A'},
/*0x1f*/    {'s','S'},
/*0x20*/    {'d','D'},
/*0x21*/    {'f','F'},
/*0x22*/    {'g','G'},
/*0x23*/    {'h','H'},
/*0x24*/    {'j','J'},
/*0x25*/    {'k','K'},
/*0x26*/    {'l','L'},
/*0x27*/    {';',':'},
/*0x28*/    {'\'','"'},
/*0x29*/    {'`','~'},
/*0x2a*/    {SHIFT_L_CHAR,SHIFT_L_CHAR},
/*0x2b*/    {'\\','|'},
/*0x2c*/    {'z','Z'},
/*0x2d*/    {'x','X'},
/*0x2e*/    {'c','C'},
/*0x2f*/    {'v','V'},
/*0x30*/    {'b','B'},
/*0x31*/    {'n','N'},
/*0x32*/    {'m','M'},
/*0x33*/    {',','<'},
/*0x34*/    {'.','>'},
/*0x35*/    {'/','?'},
/*0x36*/    {SHIFT_R_CHAR,SHIFT_R_CHAR},
/*0x37*/    {'*','*'},
/*0x38*/    {ALT_L_CHAR,ALT_L_CHAR},
/*0x39*/    {' ',' '},
/*0x3a*/    {CAPS_LOCK_CHAR,CAPS_LOCK_CHAR},
};

// 键盘中断处理程序
static void intr_keyboard_handler(void){
   bool ctrl_down_last = ctrl_status;
   bool shift_down_last = shift_status;
   bool caps_lock_last = caps_lock_status;
   bool break_code;

   uint16_t scancode = inb(KBD_BUF_PORT);
    /*
    *若扫描码是 0xe0 开头的,表示此键的按下将产生多个扫描码,故结束此次中断处理函数,等待下一个扫描码
    */
   if(scancode == 0xe0){
       ext_scancode = true;
       return;
   }

   // 如果上次是以 0xe0 开头的,将扫描码合并
   if(ext_scancode){
       scancode = ((0xe000) + scancode);
       ext_scancode = false;
   }

   break_code = ((scancode & 0x0080) != 0);

    // 如果是断码
   if(break_code){
       //获取通码
       uint16_t make_code = (scancode &= 0xff7f);

       if(make_code == CTRL_L_MAKE || make_code == CTRL_R_MAKE){
           ctrl_status = false;
       }else if(make_code == SHIFT_L_MAKE || make_code == SHIFT_R_MAKE){
           shift_status = false;
       }else if(make_code == ALT_L_MAKE || make_code == ALT_R_MAKE){
           alt_status = false;
       }
       return;
   }

    if((scancode > 0x00 && scancode < 0x3b) || (scancode == ALT_R_MAKE)
        || scancode == CTRL_R_MAKE) {
        bool shift = false;
        // 判断是否与shift组合,用来在一维数组中索引对应的字符
        /******
         * 代表两个字母的按键:
         * 0x0e   数字0-9 字符 - =
         * 0x29  ` ~
         * 0x1a [ {
         * 0x1b ] }
         * 0x2b \ |
         * 0x27 ; :
         * 0x28 ' "
         * 0x33 ,<
         * 0x34 .>
         * 0x35 /?
         */
        if((scancode < 0x0e) || (scancode == 0x29) || (scancode == 0x1a) ||
           (scancode == 0x1b) || (scancode == 0x2b) || (scancode == 0x28)
            || (scancode == 0x33) || (scancode == 0x34) || (scancode==0x35)){
            if(shift_down_last){
                shift = true;
            }
        }else { // 默认为字母键
            if(shift_down_last && caps_lock_last){
                shift = false;
            }else if(shift_down_last || caps_lock_last){
                shift = true;
            }else {
                shift = false;
            }
        }

        uint8_t index = (scancode &= 0x00ff);

        char cur_char = keymap[index][shift];

        // 处理ASCII码不为0
        if(cur_char){
            //put_char(cur_char);
            if(!ioq_full(&kbd_buf)){
                put_char(cur_char);
                ioq_putchar(&kbd_buf, cur_char);
            }
            return;
        }
        // 记录本次是否按下了下面几类控制键,供下次键入时判断组合键
        if(scancode == CTRL_L_MAKE || scancode == CTRL_R_MAKE){
            ctrl_status = true;
        }else if(scancode == SHIFT_L_MAKE || scancode == SHIFT_R_MAKE){
            shift_status = true;
        }else if(scancode == ALT_R_MAKE || scancode == ALT_L_MAKE){
            alt_status = true;
        }else if(scancode == CAPS_LOCK_MAKE){
            caps_lock_status = !caps_lock_status;
        }
    }else { // unknown
        put_str(scancode);
        put_str("unknown key\n");
    }

}
/*
 //put_char('k');
    //inb(KBD_BUF_PORT);
    uint8_t scancode = inb(KBD_BUF_PORT);
    put_int(scancode);
    return;
*/


// 键盘初始化
void keyboard_init(void){
    put_str("keyboard init start \n");
    ioqueue_init(&kbd_buf);
    register_handler(0x21, intr_keyboard_handler);
    put_str("keyboard init done\n");
}



/*0x3b*/  //  {0,0},     // F1
/*0x3c*/  //  {0,0},     // F2
/*0x3d*/  //  {0,0},     // F3
/*0x3e*/  //  {0,0},      //F4
/*0x3f*/  //  {0,0},      //F5
/*0x40*/  //  {0,0},      //F6
/*0x41*/  //  {0,0},      //F7
/*0x42*/  //  {0,0},      //F8
/*0x43*/  //  {0,0},      //F9
/*0x44*/  //  {0,0},      //F10
/*0x45*/  //  {0,0},      //numlock
/*0x46*/  //  {0,0},      //Scroll Lock
/*0x47*/  //  {0,0},      //7 Home
/*0x48*/  //  {0,0},      //8 Up
/*0x49*/  //  {0,0},      //9 pageUp
/*0x4a*/  //  {0,0},      //
/*0x4b*/  //  {0,0},      //4left
/*0x4c*/  //  {0,0},      //5
/*0x4d*/  //  {0,0},      //6 Right
/*0x4e*/  //  {0,0},      //+
/*0x4f*/  //  {0,0},      //1 End
/*0x50*/  //  {0,0},      //2 Down
/*0x51*/  //  {0,0},      //3 PageON
/*0x52*/  //  {0,0},      //0 Ins
/*0x53*/  //  {0,0},      //. Del
/*0x57*/  //  {0,0},      // F11
/*0x58*/  //  {0,0},      // F12
