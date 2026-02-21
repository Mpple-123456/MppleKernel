// MppleOS Kernel v1.0 (Freestanding version for QEMU)
#ifndef KERNEL_H
#define KERNEL_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define VERSION "1.0"

// VGA 文本模式参数
#define VGA_WIDTH  80
#define VGA_HEIGHT 25
#define VGA_MEMORY ((uint16_t*)0xB8000)

// 颜色定义
#define VGA_COLOR_BLACK         0
#define VGA_COLOR_BLUE           1
#define VGA_COLOR_GREEN          2
#define VGA_COLOR_CYAN           3
#define VGA_COLOR_RED            4
#define VGA_COLOR_MAGENTA        5
#define VGA_COLOR_BROWN          6
#define VGA_COLOR_LIGHT_GREY     7
#define VGA_COLOR_DARK_GREY      8
#define VGA_COLOR_LIGHT_BLUE     9
#define VGA_COLOR_LIGHT_GREEN   10
#define VGA_COLOR_LIGHT_CYAN    11
#define VGA_COLOR_LIGHT_RED     12
#define VGA_COLOR_LIGHT_MAGENTA 13
#define VGA_COLOR_LIGHT_BROWN   14
#define VGA_COLOR_WHITE         15

// 默认颜色
#define DEFAULT_COLOR (VGA_COLOR_BLACK << 4 | VGA_COLOR_LIGHT_GREY)

// 全局变量声明
extern uint16_t* vga_buffer;
extern int cursor_x;
extern int cursor_y;
extern uint8_t current_color;
extern bool TASK_ID[256];
extern bool poweron;

// 字符串函数声明
size_t strlen(const char* s);
int strcmp(const char* a, const char* b);
int strncmp(const char* a, const char* b, size_t n);
char* strcpy(char* dest, const char* src);
void itoa(int num, char* str);

// VGA 输出函数声明
void set_color(uint8_t color);
void putchar_at(char c, int x, int y, uint8_t color);
void clear();
void scroll();
void putchar(char c);
void print(const char* s);
void out(const char* s);

// 键盘函数声明
uint8_t inb(uint16_t port);
void outb(uint16_t port, uint8_t val);
void outw(uint16_t port, uint16_t val);
char getchar();
void read_line(char* buffer, int max_len);

// 延时函数
void sleep(int ms);

// 内核功能函数声明
void error(const char* msg);
void poweroff();
void shutdown(int sec);
void reboot(int sec);
void start_task(int id);
void kill_task(int id);
void task_manager();
void run(const char* cmd);
void test();

// 内核入口声明
extern "C" void kernel_main();

#endif // KERNEL_H