#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// ==================== 硬件参数 ====================
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_MEMORY ((uint16_t*)0xB8000)
#define VERSION "Mpple Kernel v1.0.2"

static volatile uint16_t* const vga = VGA_MEMORY;
static int cursor_x = 0;
static int cursor_y = 0;
static uint8_t current_color = 0x07;

// 键盘修饰键状态
static bool shift_pressed = false;
static bool caps_locked = false;

// ==================== 端口 I/O ====================
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outb(uint16_t port, uint8_t val) {
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline void outw(uint16_t port, uint16_t val) {
    asm volatile("outw %0, %1" : : "a"(val), "Nd"(port));
}

// ==================== 光标和屏幕输出 ====================
#define UPDATE_CURSOR() \
    do { \
        uint16_t pos = cursor_y * VGA_WIDTH + cursor_x; \
        outb(0x3D4, 14); \
        outb(0x3D5, pos >> 8); \
        outb(0x3D4, 15); \
        outb(0x3D5, pos & 0xFF); \
    } while(0)

#define CLEAR() \
    do { \
        for (int _y = 0; _y < VGA_HEIGHT; _y++) \
            for (int _x = 0; _x < VGA_WIDTH; _x++) \
                vga[_y * VGA_WIDTH + _x] = (current_color << 8) | ' '; \
        cursor_x = 0; cursor_y = 0; \
        UPDATE_CURSOR(); \
    } while(0)

#define PUTCHAR_AT(c, x, y, color) \
    do { \
        if ((x) >= 0 && (x) < VGA_WIDTH && (y) >= 0 && (y) < VGA_HEIGHT) \
            vga[(y) * VGA_WIDTH + (x)] = ((color) << 8) | (uint8_t)(c); \
    } while(0)

#define PUTCHAR(c) \
    do { \
        char _c = (c); \
        if (_c == '\b') { \
            if (cursor_x > 0) { \
                cursor_x--; \
                PUTCHAR_AT(' ', cursor_x, cursor_y, current_color); \
            } else if (cursor_y > 0) { \
                cursor_y--; \
                cursor_x = VGA_WIDTH - 1; \
                PUTCHAR_AT(' ', cursor_x, cursor_y, current_color); \
            } \
        } else if (_c == '\n') { \
            cursor_x = 0; cursor_y++; \
        } else if (_c == '\r') { \
            cursor_x = 0; \
        } else { \
            PUTCHAR_AT(_c, cursor_x, cursor_y, current_color); \
            cursor_x++; \
        } \
        if (cursor_x >= VGA_WIDTH) { cursor_x = 0; cursor_y++; } \
        if (cursor_y >= VGA_HEIGHT) { \
            CLEAR(); \
        } \
        UPDATE_CURSOR(); \
    } while(0)

#define PRINT(str) \
    do { \
        const char* _p = (str); \
        while (*_p) PUTCHAR(*_p++); \
    } while(0)

#define OUT(str) do { PRINT(str); PUTCHAR('\n'); } while(0)

// ==================== 键盘扫描码映射 ====================
static const char base_map[128] = {
    0,0,'1','2','3','4','5','6','7','8','9','0','-','=','\b',0,
    'q','w','e','r','t','y','u','i','o','p','[',']','\n',0,'a','s',
    'd','f','g','h','j','k','l',';','\'','`',0,'\\','z','x','c','v',
    'b','n','m',',','.','/',0,'*',0,' ',0,0,0,0,0,0
};

static const char shift_map[128] = {
    0,0,'!','@','#','$','%','^','&','*','(',')','_','+','\b',0,
    'Q','W','E','R','T','Y','U','I','O','P','{','}','\n',0,'A','S',
    'D','F','G','H','J','K','L',':','"','~',0,'|','Z','X','C','V',
    'B','N','M','<','>','?',0,'*',0,' ',0,0,0,0,0,0
};

// ==================== 错误处理 ====================
static void panic(const char* msg) {
    // 直接操作显存，不依赖任何宏
    volatile uint16_t* video = (uint16_t*)0xB8000;
    // 清屏
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        video[i] = (0x07 << 8) | ' ';
    }
    // 显示错误信息
    const char* prefix = "KERNEL PANIC: ";
    int pos = 0;
    while (*prefix) {
        video[pos++] = (0x04 << 8) | *prefix++; // 红色
    }
    while (*msg) {
        video[pos++] = (0x0F << 8) | *msg++;    // 白色
    }
    // 延时约3秒（粗略）
    for (volatile int i = 0; i < 10000000; i++);
    // 重启系统（键盘控制器复位）
    asm volatile("cli");
    while (inb(0x64) & 0x02); // 等待键盘控制器空闲
    outb(0x64, 0xFE);         // 发送复位脉冲
    asm volatile("hlt");
    while (1);
}

#define ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            panic(msg); \
        } \
    } while(0)

// 读取一个字符（阻塞）
static inline char getchar() {
    uint8_t scancode;
    while ((inb(0x64) & 1) == 0);
    scancode = inb(0x60);

    if (scancode == 0x2A || scancode == 0x36) { shift_pressed = true; return 0; }
    if (scancode == 0xAA || scancode == 0xB6) { shift_pressed = false; return 0; }
    if (scancode == 0x3A) {
        caps_locked = !caps_locked;
        volatile uint16_t* vga = (uint16_t*)0xB8000;
        if (caps_locked) {
            vga[79] = (0x0F << 8) | 'C';
            vga[78] = (0x0F << 8) | 'S';
            vga[77] = (0x0F << 8) | 'A';
            vga[76] = (0x0F << 8) | 'P';
        } else {
            vga[79] = (0x0F << 8) | ' ';
            vga[78] = (0x0F << 8) | ' ';
            vga[77] = (0x0F << 8) | ' ';
            vga[76] = (0x0F << 8) | ' ';
        }
        return 0;
    }
    if (scancode & 0x80) return 0;
    if (scancode >= 128) return 0;
    char base = base_map[scancode];
    char shifted = shift_map[scancode];
    if (base == 0) return 0;
    if ((base >= 'a' && base <= 'z') || (base >= 'A' && base <= 'Z')) {
        if (shift_pressed != caps_locked) return shifted;
        else return base;
    } else {
        if (shift_pressed) return shifted ? shifted : base;
        else return base;
    }
}

// 读取一行输入
static inline void read_line(char* buffer, int max_len) {
    ASSERT(buffer != NULL, "read_line: buffer is NULL");
    ASSERT(max_len > 0, "read_line: max_len <= 0");
    int i = 0;
    while (i < max_len - 1) {
        char c = getchar();
        if (c == 0) continue;
        if (c == '\n') {
            buffer[i] = '\0';
            PUTCHAR('\n');
            break;
        } else if (c == '\b') {
            if (i > 0) {
                i--;
                PUTCHAR('\b');
            }
        } else {
            buffer[i++] = c;
            PUTCHAR(c);
        }
    }
    if (i == max_len - 1) buffer[i] = '\0';
}

// ==================== 字符串辅助 ====================
static inline int strequal(const char* a, const char* b) {
    ASSERT(a != NULL && b != NULL, "strequal: NULL argument");
    while (*a && *b && *a == *b) { a++; b++; }
    return (*a == '\0' && *b == '\0');
}

// ==================== 程序管理（未使用）====================
typedef void (*prog_entry_t)(void);
struct Program {
    const char* name;
    prog_entry_t entry;
};
static const Program programs[] = {};
static const int num_programs = sizeof(programs) / sizeof(programs[0]);

// ==================== 界面绘制 ====================
// 安全的 draw 函数：忽略空行
static void draw(const char* pic[], int lines) {
    ASSERT(pic != NULL, "draw: pic is NULL");
    ASSERT(lines >= 0, "draw: lines negative");
    for (int i = 0; i < lines; i++) {
        if (pic[i] == NULL) continue;
        OUT(pic[i]);
    }
}

// 桌面图标数组
static const char* desktop_icons[] = {
    "+========================================+",
    "|               (1)POWER                 |",
    "+========================================+",
    "| +---------+  +---------+  +---------+  |",
    "| |  EXIT   |  |POWER OFF|  | REBOOT  |  |",
    "| |    |    |  |   ( )   |  |   / \\   |  |",
    "| |    v    |  |    |    |  |  |   |  |  |",
    "| |         |  |    |    |  |   \\ /   |  |",
    "| +---------+  +---------+  +---------+  |",
    "|     [1]         [2]           [3]      |",
    "|                                        |",
    "+========================================+"
};
static const int desktop_icon_lines = sizeof(desktop_icons) / sizeof(desktop_icons[0]);

// 桌面主函数
static void Desktop() {
    while (1) {
        CLEAR();
        for (int i = 0; i < desktop_icon_lines; i++) {
            if (desktop_icons[i] != NULL) {
                OUT(desktop_icons[i]);
            }
        }
        char choice = 0;
        while (choice == 0) {
            choice = getchar();
        }

        if (choice == '1') {
            return; // 返回命令行
        } else if (choice == '2') {
            OUT("System powering off...");
            for (int i = 0; i < 5000000; i++) asm volatile("pause");
            outw(0x604, 0x2000);
            outb(0x64, 0xFE);
            asm volatile("cli; hlt");
            while (1);
        } else if (choice == '3') {
            OUT("System rebooting...");
            for (int i = 0; i < 5000000; i++) asm volatile("pause");
            while (inb(0x64) & 0x02);
            outb(0x64, 0xFE);
            asm volatile("cli; hlt");
            while (1);
        } else {
            OUT("Invalid choice. Press any key to continue...");
            while (getchar() == 0);
        }
    }
}

// 显示启动 Logo
static void draw_logo() {
    const char* logo[] = {
        "      |",
        "  /\\  |  /\\",
        " /--\\-|-/--\\",
        "/    \\|/    \\",
        " Mpple Kernel",
        "==================="
    };
    for (int i = 0; i < 6; i++) {
        OUT(logo[i]);
    }
}

// ==================== 内核入口 ====================
extern "C" void kernel_main() {
    // 检查内核是否正常启动（可选）
    // 例如，可以检查某些关键变量是否合理，但此处省略

    CLEAR();
    draw_logo();

    for (int i = 0; i < 3000000; i++) asm volatile("pause");
    shift_pressed = false;
    caps_locked = false;
    OUT(VERSION);
    OUT("Type 'help' for commands.");

    char cmd_buf[128];
    while (1) {
        PRINT("> ");
        read_line(cmd_buf, sizeof(cmd_buf));

        if (cmd_buf[0] == '\0') continue;

        if (strequal(cmd_buf, "help")) {
            OUT("help     - Show this help");
            OUT("clear    - Clear screen");
            OUT("echo     - Echo a message (echo <text>)");
            OUT("ver      - Show version");
            OUT("poweroff - Shut down the system");
            OUT("shutdown - Same as poweroff");
            OUT("reboot   - Reboot the system");
            OUT("run      - Run a program (run <progname>)");
            OUT("desktop  - Enter desktop environment");
            continue;
        }

        if (strequal(cmd_buf, "clear")) {
            CLEAR();
            continue;
        }

        if (strequal(cmd_buf, "ver")) {
            OUT(VERSION);
            continue;
        }

        if (cmd_buf[0] == 'e' && cmd_buf[1] == 'c' && cmd_buf[2] == 'h' && cmd_buf[3] == 'o' && cmd_buf[4] == ' ') {
            OUT(cmd_buf + 5);
            continue;
        }

        if (strequal(cmd_buf, "poweroff") || strequal(cmd_buf, "shutdown")) {
            OUT("System powering off...");
            for (int i = 0; i < 5000000; i++) asm volatile("pause");
            outw(0x604, 0x2000);
            outb(0x64, 0xFE);
            asm volatile("cli; hlt");
            while (1);
        }

        if (strequal(cmd_buf, "reboot")) {
            OUT("System rebooting...");
            for (int i = 0; i < 5000000; i++) asm volatile("pause");
            while (inb(0x64) & 0x02);
            outb(0x64, 0xFE);
            asm volatile("cli; hlt");
            while (1);
        }

        if (cmd_buf[0] == 'r' && cmd_buf[1] == 'u' && cmd_buf[2] == 'n' && cmd_buf[3] == ' ') {
            const char* prog_name = cmd_buf + 4;
            int found = 0;
            for (int i = 0; i < num_programs; i++) {
                if (strequal(prog_name, programs[i].name)) {
                    OUT("Starting program: ");
                    OUT(programs[i].name);
                    if (programs[i].entry) programs[i].entry();
                    OUT("Program returned.");
                    found = 1;
                    break;
                }
            }
            if (!found) OUT("Program not found.");
            continue;
        }

        if (strequal(cmd_buf, "desktop")) {
            Desktop();
            continue;
        }

        if (strequal(cmd_buf, "RESETKB")) {
            shift_pressed = false;
            caps_locked = false;
            OUT("Keyboard state reset.");
            continue;
        }

        OUT("Unknown command. Type 'help'.");
    }
}