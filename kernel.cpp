#include <stdint.h>
#include <stdbool.h>

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_MEMORY ((uint16_t*)0xB8000)

static volatile uint16_t* const vga = VGA_MEMORY;
static int cursor_x = 0;
static int cursor_y = 0;
static uint8_t current_color = 0x07;  // 灰色

// 键盘修饰键状态
static bool shift_pressed = false;
static bool caps_locked = false;

// 清屏宏
#define CLEAR() \
    do { \
        for (int _y = 0; _y < VGA_HEIGHT; _y++) \
            for (int _x = 0; _x < VGA_WIDTH; _x++) \
                vga[_y * VGA_WIDTH + _x] = (current_color << 8) | ' '; \
        cursor_x = 0; cursor_y = 0; \
    } while(0)

// 在指定位置输出字符
#define PUTCHAR_AT(c, x, y, color) \
    do { \
        if ((x) >= 0 && (x) < VGA_WIDTH && (y) >= 0 && (y) < VGA_HEIGHT) \
            vga[(y) * VGA_WIDTH + (x)] = ((color) << 8) | (uint8_t)(c); \
    } while(0)

// 输出单个字符（处理换行、回车、退格）
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
    } while(0)

// 输出字符串
#define PRINT(str) \
    do { \
        const char* _p = (str); \
        while (*_p) PUTCHAR(*_p++); \
    } while(0)

// 输出字符串并换行
#define OUT(str) do { PRINT(str); PUTCHAR('\n'); } while(0)

// 端口 I/O 内联函数
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

// 键盘扫描码映射表（US 布局）
static const char base_map[128] = {
    // 0x00-0x0F
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b', 0,
    // 0x10-0x1F
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0, 'a', 's',
    // 0x20-0x2F
    'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\', 'z', 'x', 'c', 'v',
    // 0x30-0x3F
    'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' ', 0, 0, 0, 0, 0, 0
};

static const char shift_map[128] = {
    // 0x00-0x0F
    0, 0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b', 0,
    // 0x10-0x1F
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n', 0, 'A', 'S',
    // 0x20-0x2F
    'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0, '|', 'Z', 'X', 'C', 'V',
    // 0x30-0x3F
    'B', 'N', 'M', '<', '>', '?', 0, '*', 0, ' ', 0, 0, 0, 0, 0, 0
};

// 读取一个字符（阻塞，返回ASCII，0表示无效）
static inline char getchar() {
    uint8_t scancode;
    while ((inb(0x64) & 1) == 0);
    scancode = inb(0x60);

    // 处理修饰键状态
    if (scancode == 0x2A || scancode == 0x36) { // 左Shift或右Shift按下
        shift_pressed = true;
        return 0;
    }
    if (scancode == 0xAA || scancode == 0xB6) { // 左Shift或右Shift释放
        shift_pressed = false;
        return 0;
    }
    if (scancode == 0x3A) { // Caps Lock 按下
        caps_locked = !caps_locked;
        // 在屏幕右上角显示 Caps 状态
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
    if (scancode & 0x80) {
        // 其他键释放，忽略
        return 0;
    }

    // 处理可打印键
    if (scancode >= 128) return 0;
    char base = base_map[scancode];
    char shifted = shift_map[scancode];
    if (base == 0) return 0;

    // 判断是否为字母
    if ((base >= 'a' && base <= 'z') || (base >= 'A' && base <= 'Z')) {
        // 字母大小写由 shift XOR caps 决定
        if (shift_pressed != caps_locked) {
            return shifted; // 大写
        } else {
            return base;    // 小写
        }
    } else {
        // 非字母：shift 决定使用上档还是下档
        if (shift_pressed) {
            return shifted ? shifted : base;
        } else {
            return base;
        }
    }
}

// 读取一行输入（存入buffer，最多max_len-1字符）
static inline void read_line(char* buffer, int max_len) {
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

// 程序入口类型
typedef void (*prog_entry_t)(void);

// 程序信息结构
struct Program {
    const char* name;
    prog_entry_t entry;
};

// 预定义的程序列表（必须与合并到内核映像的顺序和地址一致）
static const Program programs[] = {
    //{ "calc", (prog_entry_t)0x40000 },
    // 没做
};
static const int num_programs = sizeof(programs) / sizeof(programs[0]);

// 简单的字符串比较函数（返回1表示相等）
static inline int strequal(const char* a, const char* b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return (*a == '\0' && *b == '\0');
}

// 内核入口
extern "C" void kernel_main() {
    shift_pressed = false;
    caps_locked = false;
    CLEAR();
    OUT("MppleOS Kernel v1.0");
    OUT("Type 'help' for commands.");

    char cmd_buf[128];
    while (1) {
        PRINT("> ");
        read_line(cmd_buf, sizeof(cmd_buf));

        if (cmd_buf[0] == '\0') continue;

        // help
        if (strequal(cmd_buf, "help")) {
            OUT("help     - Show this help");
            OUT("clear    - Clear screen");
            OUT("echo     - Echo a message (echo <text>)");
            OUT("ver      - Show version");
            OUT("poweroff - Shut down the system");
            OUT("shutdown - Same as poweroff");
            OUT("reboot   - Reboot the system");
            OUT("run      - Run a program (run <progname>)");
            continue;
        }

        // clear
        if (strequal(cmd_buf, "clear")) {
            CLEAR();
            continue;
        }

        // ver
        if (strequal(cmd_buf, "ver")) {
            OUT("MppleOS Kernel v1.0");
            continue;
        }

        // echo
        if (cmd_buf[0] == 'e' && cmd_buf[1] == 'c' && cmd_buf[2] == 'h' && cmd_buf[3] == 'o' && cmd_buf[4] == ' ') {
            OUT(cmd_buf + 5);
            continue;
        }

        // poweroff
        if (strequal(cmd_buf, "poweroff")) {
            OUT("System powering off...");
            for (int i = 0; i < 5000000; i++) asm volatile("pause");
            outw(0x604, 0x2000);  // QEMU 关机
            outb(0x64, 0xFE);      // 备用复位
            asm volatile("cli; hlt");
            while (1);
        }

        // shutdown
        if (strequal(cmd_buf, "shutdown")) {
            OUT("System shutting down...");
            for (int i = 0; i < 5000000; i++) asm volatile("pause");
            outw(0x604, 0x2000);
            outb(0x64, 0xFE);
            asm volatile("cli; hlt");
            while (1);
        }

        // reboot
        if (strequal(cmd_buf, "reboot")) {
            OUT("System rebooting...");
            for (int i = 0; i < 5000000; i++) asm volatile("pause");
            while (inb(0x64) & 0x02);
            outb(0x64, 0xFE);
            asm volatile("cli; hlt");
            while (1);
        }

        // run 命令
        if (cmd_buf[0] == 'r' && cmd_buf[1] == 'u' && cmd_buf[2] == 'n' && cmd_buf[3] == ' ') {
            const char* prog_name = cmd_buf + 4;
            int found = 0;
            for (int i = 0; i < num_programs; i++) {
                if (strequal(prog_name, programs[i].name)) {
                    OUT("Starting program: ");
                    OUT(programs[i].name);
                    programs[i].entry();  // 调用程序
                    // 如果程序返回，会执行到这里
                    OUT("Program returned.");
                    found = 1;
                    break;
                }
            }
            if (!found) {
                OUT("Program not found.");
            }
            continue;
        }
        if (strequal(cmd_buf, "RESETKB")) {
            shift_pressed = false;
            caps_locked = false;
            OUT("Keyboard state reset.");
            continue;
        }
        // 未知命令
        OUT("Unknown command. Type 'help'.");
    }
}