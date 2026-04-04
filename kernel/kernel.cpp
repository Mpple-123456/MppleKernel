#include <stdint.h>
#include <stdbool.h>

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_MEMORY ((uint16_t*)0xB8000)
#define VERSION "Mpple Kernel v1.0.5"

#ifdef GRUB
#define MULTIBOOT_HEADER_MAGIC  0x1BADB002
#define MULTIBOOT_HEADER_FLAGS   0x00000003
#define MULTIBOOT_CHECKSUM       ((uint32_t)(0 - (MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)))

__attribute__((section(".multiboot")))
struct multiboot_header {
    uint32_t magic;
    uint32_t flags;
    uint32_t checksum;
    uint32_t header_addr;
    uint32_t load_addr;
    uint32_t load_end_addr;
    uint32_t bss_end_addr;
    uint32_t entry_addr;
} multiboot_header = {
    MULTIBOOT_HEADER_MAGIC,
    MULTIBOOT_HEADER_FLAGS,
    MULTIBOOT_CHECKSUM,
    0, 0, 0, 0, 0
};
#endif

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

static inline void io_wait() {
    inb(0x80);
}

static void init_keyboard() {
    while (inb(0x64) & 2);
    outb(0x64, 0xAE);
    io_wait();
    while (inb(0x64) & 2);
    outb(0x60, 0xF4);
    io_wait();
}

static volatile uint16_t* vga = (volatile uint16_t*)VGA_MEMORY;
static uint16_t cx = 0, cy = 0;

static void clear() {
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga[i] = 0x0F00 | ' ';
    }
    cx = 0; cy = 0;
    outb(0x3D4, 14);
    outb(0x3D5, 0);
    outb(0x3D4, 15);
    outb(0x3D5, 0);
}

static void puts(const char* s) {
    while (*s) {
        if (*s == '\n') {
            cx = 0;
            cy++;
        } else {
            vga[cy * VGA_WIDTH + cx] = 0x0F00 | *s;
            cx++;
            if (cx >= VGA_WIDTH) {
                cx = 0;
                cy++;
            }
        }
        s++;
    }
    uint16_t pos = cy * VGA_WIDTH + cx;
    outb(0x3D4, 14);
    outb(0x3D5, pos >> 8);
    outb(0x3D4, 15);
    outb(0x3D5, pos);
}

static void putc(char c) {
    if (c == '\n') {
        cx = 0;
        cy++;
    } else {
        vga[cy * VGA_WIDTH + cx] = 0x0F00 | c;
        cx++;
    }
    if (cx >= VGA_WIDTH) {
        cx = 0;
        cy++;
    }
    uint16_t pos = cy * VGA_WIDTH + cx;
    outb(0x3D4, 14);
    outb(0x3D5, pos >> 8);
    outb(0x3D4, 15);
    outb(0x3D5, pos);
}

static uint8_t wait_key() {
    while (1) {
        if (inb(0x64) & 1) {
            uint8_t k = inb(0x60);
            if ((k & 0x80) == 0) {
                return k;
            }
        }
    }
}

static char scancode_to_ascii(uint8_t sc) {
    switch (sc) {
        case 0x02: return '1';
        case 0x03: return '2';
        case 0x04: return '3';
        case 0x05: return '4';
        case 0x06: return '5';
        case 0x07: return '6';
        case 0x08: return '7';
        case 0x09: return '8';
        case 0x0A: return '9';
        case 0x0B: return '0';
        case 0x0C: return '-';
        case 0x0D: return '+';
        case 0x0E: return '\b';
        case 0x0F: return '\t';
        case 0x10: return 'q';
        case 0x11: return 'w';
        case 0x12: return 'e';
        case 0x13: return 'r';
        case 0x14: return 't';
        case 0x15: return 'y';
        case 0x16: return 'u';
        case 0x17: return 'i';
        case 0x18: return 'o';
        case 0x19: return 'p';
        case 0x1C: return '\n';
        case 0x1E: return 'a';
        case 0x1F: return 's';
        case 0x20: return 'd';
        case 0x21: return 'f';
        case 0x22: return 'g';
        case 0x23: return 'h';
        case 0x24: return 'j';
        case 0x25: return 'k';
        case 0x26: return 'l';
        case 0x2C: return 'z';
        case 0x2D: return 'x';
        case 0x2E: return 'c';
        case 0x2F: return 'v';
        case 0x30: return 'b';
        case 0x31: return 'n';
        case 0x32: return 'm';
        case 0x39: return ' ';
        case 0x27: return ';';
        case 0x28: return '\'';
        case 0x33: return ',';
        case 0x34: return '.';
        case 0x35: return '/';
        case 0x4A: return '-';
        case 0x4E: return '+';
        case 0x37: return '*';
        case 0x5A: return '\n';
        default: return 0;
    }
}

static void reboot() {
    puts("Rebooting...\n");
    while (inb(0x64) & 2);
    outb(0x64, 0xFE);
    asm volatile("cli; hlt");
}

static void shutdown() {
    puts("Powering off...\n");
    outw(0x604, 0x2000);
    asm volatile("cli; hlt");
}

static void logo() {
    puts("  /\\  |  /\\  \n");
    puts(" /--\\-|-/--\\ \n");
    puts("/    \\|/    \\\n");
    puts(" Mpple Kernel\n");
    puts("===================\n");
}

static bool streq(const char* a, const char* b) {
    while (*a && *b) {
        if (*a != *b) return false;
        a++; b++;
    }
    return *a == *b;
}

static void help() {
    puts("Commands:\n");
    puts("  help    - Show this\n");
    puts("  clear   - Clear screen\n");
    puts("  ver     - Show version\n");
    puts("  reboot  - Reboot\n");
    puts("  poweroff - Shutdown\n");
    puts("  desktop - GUI mode\n");
}

struct Window {
    int x, y, w, h;
    const char* title;
    bool active;
    bool visible;
};

#define MAX_WINDOWS 4
static Window windows[MAX_WINDOWS];
static int active_window = -1;
static int num_windows = 0;
static bool start_menu_open = false;
static int selected_item = 0;

static void draw_desktop_bg() {
    for (int y = 0; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            if (y < VGA_HEIGHT - 1) {
                vga[y * VGA_WIDTH + x] = (0x10 << 8) | ' ';
            } else {
                vga[y * VGA_WIDTH + x] = (0x70 << 8) | ' ';
            }
        }
    }
}

static void draw_desktop_icon(int x, int y, const char* name, char icon) {
    vga[y * VGA_WIDTH + x + 1] = (0x0E << 8) | icon;
    for (int i = 0; name[i] && i < 10; i++) {
        vga[(y + 1) * VGA_WIDTH + x + i] = (0x0F << 8) | name[i];
    }
}

static void draw_taskbar() {
    int ty = VGA_HEIGHT - 1;
    for (int x = 0; x < VGA_WIDTH; x++) {
        vga[ty * VGA_WIDTH + x] = (0x70 << 8) | ' ';
    }
    
    for (int i = 0; i < 7; i++) {
        vga[ty * VGA_WIDTH + i] = (0x0C << 8) | '_';
    }
    vga[ty * VGA_WIDTH + 0] = (0x0C << 8) | 'S';
    
    for (int i = 0; i < num_windows; i++) {
        if (windows[i].visible) {
            int tx = 10 + i * 20;
            uint8_t color = windows[i].active ? 0x1E : 0x0E;
            vga[ty * VGA_WIDTH + tx] = (color << 8) | '[';
            int len = 0;
            while (windows[i].title[len] && tx + len + 1 < VGA_WIDTH - 1) {
                vga[ty * VGA_WIDTH + tx + 1 + len] = (color << 8) | windows[i].title[len];
                len++;
            }
            if (tx + len + 1 < VGA_WIDTH) {
                vga[ty * VGA_WIDTH + tx + len + 1] = (color << 8) | ']';
            }
        }
    }
    
    for (int x = 10; x < VGA_WIDTH; x++) {
        vga[(ty - 1) * VGA_WIDTH + x] = (0x08 << 8) | ' ';
    }
}

static void draw_window_win(const Window* win) {
    if (!win->visible) return;
    
    uint8_t title_color = win->active ? 0x1F : 0x1B;
    uint8_t border_color = win->active ? 0x17 : 0x13;
    
    for (int i = 0; i < win->w; i++) {
        vga[win->y * VGA_WIDTH + win->x + i] = (title_color << 8) | ' ';
    }
    
    int title_len = 0;
    while (win->title[title_len] && title_len < win->w - 4) title_len++;
    for (int i = 0; i < title_len; i++) {
        vga[win->y * VGA_WIDTH + win->x + 2 + i] = (title_color << 8) | win->title[i];
    }
    
    vga[win->y * VGA_WIDTH + win->x] = (border_color << 8) | '+';
    vga[win->y * VGA_WIDTH + win->x + win->w - 1] = (border_color << 8) | '+';
    
    for (int i = 1; i < win->w - 1; i++) {
        vga[win->y * VGA_WIDTH + win->x + i] = (title_color << 8) | ' ';
        vga[(win->y + win->h - 1) * VGA_WIDTH + win->x + i] = (border_color << 8) | '-';
    }
    
    for (int i = 1; i < win->h - 1; i++) {
        vga[(win->y + i) * VGA_WIDTH + win->x] = (border_color << 8) | '|';
        vga[(win->y + i) * VGA_WIDTH + win->x + win->w - 1] = (border_color << 8) | '|';
        for (int j = 1; j < win->w - 1; j++) {
            vga[(win->y + i) * VGA_WIDTH + win->x + j] = (0x1F << 8) | ' ';
        }
    }
    
    vga[win->y * VGA_WIDTH + win->x + win->w - 2] = (0x0C << 8) | 'X';
    vga[win->y * VGA_WIDTH + win->x + win->w - 1] = (border_color << 8) | '+';
    vga[(win->y + win->h - 1) * VGA_WIDTH + win->x] = (border_color << 8) | '+';
    vga[(win->y + win->h - 1) * VGA_WIDTH + win->x + win->w - 1] = (border_color << 8) | '+';
}

static void draw_start_menu() {
    if (!start_menu_open) return;
    
    int mx = 0, my = VGA_HEIGHT - 14;
    int mw = 20, mh = 12;
    
    for (int i = 0; i < mw; i++) {
        vga[my * VGA_WIDTH + mx + i] = (0x30 << 8) | ' ';
    }
    vga[my * VGA_WIDTH + mx] = (0x30 << 8) | '+';
    vga[my * VGA_WIDTH + mx + mw - 1] = (0x30 << 8) | '+';
    
    for (int i = 1; i < mh - 1; i++) {
        vga[(my + i) * VGA_WIDTH + mx] = (0x30 << 8) | '|';
        for (int j = 1; j < mw - 1; j++) {
            vga[(my + i) * VGA_WIDTH + mx + j] = (0x30 << 8) | ' ';
        }
        vga[(my + i) * VGA_WIDTH + mx + mw - 1] = (0x30 << 8) | '|';
    }
    
    for (int i = 0; i < mw; i++) {
        vga[(my + mh - 1) * VGA_WIDTH + mx + i] = (0x30 << 8) | '-';
    }
    vga[(my + mh - 1) * VGA_WIDTH + mx] = (0x30 << 8) | '+';
    vga[(my + mh - 1) * VGA_WIDTH + mx + mw - 1] = (0x30 << 8) | '+';
    
    const char* items[] = {"Calculator", "Notepad", "About", "Exit"};
    for (int i = 0; i < 4; i++) {
        uint8_t color = (i == selected_item) ? 0x30 : 0x0F;
        vga[(my + 2 + i * 2) * VGA_WIDTH + mx + 2] = (color << 8) | '>';
        for (int j = 0; items[i][j]; j++) {
            vga[(my + 2 + i * 2) * VGA_WIDTH + mx + 4 + j] = (color << 8) | items[i][j];
        }
    }
    
    vga[(my + 1) * VGA_WIDTH + mx + 2] = (0x0F << 8) | 'S';
    vga[(my + 1) * VGA_WIDTH + mx + 3] = (0x0F << 8) | 't';
    vga[(my + 1) * VGA_WIDTH + mx + 4] = (0x0F << 8) | 'a';
    vga[(my + 1) * VGA_WIDTH + mx + 5] = (0x0F << 8) | 'r';
    vga[(my + 1) * VGA_WIDTH + mx + 6] = (0x0F << 8) | 't';
}

static int strlen(const char* s) {
    int len = 0;
    while (s[len]) len++;
    return len;
}

static void draw_calc_in_window() {
    for (int i = 0; i < num_windows; i++) {
        if (windows[i].visible && windows[i].title[0] == 'C') {
            int wx = windows[i].x + 1;
            int wy = windows[i].y + 1;
            
            const char* label = "Display: 0 ";
            for (int j = 0; label[j]; j++) {
                vga[wy * VGA_WIDTH + wx + j] = (0x09 << 8) | label[j];
            }
            
            const char* help_text[] = {"1-9,0: Numbers", "+-*/: Ops", "Enter: =", "Esc/c: Clear", "q: Close"};
            for (int line = 0; line < 5; line++) {
                for (int j = 0; help_text[line][j]; j++) {
                    vga[(wy + 2 + line) * VGA_WIDTH + wx + j] = (0x0F << 8) | help_text[line][j];
                }
            }
            break;
        }
    }
}

static void draw_desktop_icons() {
    draw_desktop_icon(2, 2, "Calculator", 'C');
    draw_desktop_icon(2, 5, "Notepad", 'N');
    draw_desktop_icon(2, 8, "About", 'A');
}

static void add_window(const char* title, int w, int h) {
    if (num_windows >= MAX_WINDOWS) return;
    windows[num_windows].title = title;
    windows[num_windows].w = w;
    windows[num_windows].h = h;
    windows[num_windows].x = 10 + num_windows * 3;
    windows[num_windows].y = 2 + num_windows * 2;
    windows[num_windows].visible = true;
    windows[num_windows].active = true;
    
    for (int i = 0; i < num_windows; i++) {
        windows[i].active = false;
    }
    active_window = num_windows;
    num_windows++;
}

static void close_active_window() {
    if (active_window < 0 || active_window >= num_windows) return;
    
    for (int i = active_window; i < num_windows - 1; i++) {
        windows[i] = windows[i + 1];
    }
    num_windows--;
    
    if (num_windows > 0 && active_window >= num_windows) {
        active_window = num_windows - 1;
    }
    if (num_windows > 0) {
        windows[active_window].active = true;
    } else {
        active_window = -1;
    }
}

static void run_calculator() {
    clear();
    puts("|==========================|\n");
    puts("|     MAPPLE CALCULATOR    |\n");
    puts("|==========================|\n");
    
    while ((inb(0x64) & 1) != 0) inb(0x60);
    
    int num1 = 0, num2 = 0, result = 0;
    char op = 0;
    char input[32] = {0};
    int input_len = 0;
    bool has_result = false;
    
    while (1) {
        puts("> ");
        input_len = 0;
        
        while (1) {
            uint8_t k = wait_key();
            if ((k & 0x80) != 0) continue;
            
            if (k == 0x01) return;
            if (k == 0x31 && scancode_to_ascii(k) == 'q') return;
            
            char c = scancode_to_ascii(k);
            
            if (c == 'c' || c == 'C') {
                num1 = num2 = result = 0;
                op = 0;
                has_result = false;
                puts("\ncleared\n");
                break;
            }
            
            if (c >= '0' && c <= '9') {
                if (input_len < 20) {
                    input[input_len++] = c;
                    putc(c);
                }
            }
            
            if (c == '+' || c == '-' || c == '*' || c == '/') {
                if (input_len > 0) {
                    int val = 0;
                    for (int i = 0; i < input_len; i++) {
                        val = val * 10 + (input[i] - '0');
                    }
                    if (has_result) num1 = result;
                    else num1 = val;
                    input_len = 0;
                }
                op = c;
                putc(c);
            }
            
            if (c == '\n') {
                if (input_len > 0) {
                    int val = 0;
                    for (int i = 0; i < input_len; i++) {
                        val = val * 10 + (input[i] - '0');
                    }
                    num2 = val;
                    input_len = 0;
                }
                
                if (op == 0 && !has_result) {
                    result = num1;
                    has_result = true;
                }
                
                if (op == '+') result = num1 + num2;
                else if (op == '-') result = num1 - num2;
                else if (op == '*') result = num1 * num2;
                else if (op == '/') {
                    if (num2 == 0) { puts("\nError: div by 0\n"); num1 = num2 = 0; op = 0; has_result = false; break; }
                    result = num1 / num2;
                }
                
                putc('=');
                puts("\nResult: ");
                if (result < 0) { putc('-'); result = -result; }
                if (result == 0) putc('0');
                else {
                    char buf[24];
                    int p = 0;
                    int t = result;
                    while (t > 0) { buf[p++] = '0' + (t % 10); t /= 10; }
                    for (int i = p - 1; i >= 0; i--) putc(buf[i]);
                }
                puts("\n");
                has_result = true;
                break;
            }
        }
    }
}

static void show_about() {
    clear();
    puts("+==========================+\n");
    puts("|     MAPPLE KERNEL        |\n");
    puts("|     Version 1.0.5        |\n");
    puts("+==========================+\n");
    puts("| A simple x86 OS kernel  |\n");
    puts("| Written in C/C++         |\n");
    puts("+==========================+\n\n");
    puts("Press Enter or Esc to return\n");
    
    while ((inb(0x64) & 1) != 0) inb(0x60);
    
    while (1) {
        uint8_t k = wait_key();
        if ((k & 0x80) != 0) continue;
        if (k == 0x01 || k == 0x1C || k == 0x5A) break;
    }
}

static void show_notepad() {
    clear();
    puts("+==========================+\n");
    puts("|     NOTEPAD              |\n");
    puts("+==========================+\n");
    puts("| Type to write...        |\n");
    puts("| Press Enter to save     |\n");
    puts("| Press Esc to exit       |\n");
    puts("+==========================+\n\n");
    
    char lines[10][80];
    int line_count = 0;
    int current_line = 0;
    lines[0][0] = 0;
    
    while ((inb(0x64) & 1) != 0) inb(0x60);
    
    while (1) {
        uint8_t k = wait_key();
        if ((k & 0x80) != 0) continue;
        
        if (k == 0x01) return;
        
        char c = scancode_to_ascii(k);
        
        if (c == '\n') {
            line_count++;
            current_line++;
            if (line_count >= 10) line_count = 9;
            lines[line_count][0] = 0;
            putc('\n');
        } else if (c == '\b') {
            if (lines[line_count][0] > 0) {
                lines[line_count][strlen(lines[line_count]) - 1] = 0;
                putc('\b');
                putc(' ');
                putc('\b');
            }
        } else if (c != 0) {
            int len = strlen(lines[line_count]);
            if (len < 78) {
                lines[line_count][len] = c;
                lines[line_count][len + 1] = 0;
                putc(c);
            }
        }
    }
}

static void desktop() {
    num_windows = 0;
    active_window = -1;
    start_menu_open = false;
    selected_item = 0;
    
    draw_desktop_bg();
    draw_desktop_icons();
    draw_taskbar();
    
    while ((inb(0x64) & 1) != 0) inb(0x60);
    
    while (1) {
        uint8_t k = wait_key();
        if ((k & 0x80) != 0) continue;
        
        if (k == 0x01) return;
        
        char c = scancode_to_ascii(k);
        
        if (c == '\n') {
            if (start_menu_open) {
                if (selected_item == 0) {
                    start_menu_open = false;
                    run_calculator();
                    draw_desktop_bg();
                    draw_desktop_icons();
                    for (int i = 0; i < num_windows; i++) {
                        draw_window_win(&windows[i]);
                    }
                    draw_taskbar();
                } else if (selected_item == 1) {
                    start_menu_open = false;
                    show_notepad();
                    draw_desktop_bg();
                    draw_desktop_icons();
                    for (int i = 0; i < num_windows; i++) {
                        draw_window_win(&windows[i]);
                    }
                    draw_taskbar();
                } else if (selected_item == 2) {
                    start_menu_open = false;
                    show_about();
                    draw_desktop_bg();
                    draw_desktop_icons();
                    for (int i = 0; i < num_windows; i++) {
                        draw_window_win(&windows[i]);
                    }
                    draw_taskbar();
                } else if (selected_item == 3) {
                    return;
                }
                selected_item = 0;
            } else {
                add_window("Calculator", 40, 15);
                draw_desktop_bg();
                draw_desktop_icons();
                for (int i = 0; i < num_windows; i++) {
                    draw_window_win(&windows[i]);
                }
                draw_taskbar();
            }
        }
        
        if (k == 0x4B) {
            if (start_menu_open) {
                selected_item = (selected_item - 1 + 4) % 4;
                draw_start_menu();
            }
        }
        if (k == 0x4D) {
            if (start_menu_open) {
                selected_item = (selected_item + 1) % 4;
                draw_start_menu();
            }
        }
        
        if (c == 's' || c == 'S') {
            start_menu_open = !start_menu_open;
            selected_item = 0;
            draw_desktop_bg();
            draw_desktop_icons();
            for (int i = 0; i < num_windows; i++) {
                draw_window_win(&windows[i]);
            }
            draw_taskbar();
            if (start_menu_open) {
                draw_start_menu();
            }
        }
        
        if (c == 'q' || c == 'Q') {
            if (active_window >= 0) {
                close_active_window();
                draw_desktop_bg();
                draw_desktop_icons();
                for (int i = 0; i < num_windows; i++) {
                    draw_window_win(&windows[i]);
                }
                draw_taskbar();
            }
        }
        
        if (k == 0x0E) {
            if (active_window >= 0) {
                close_active_window();
                draw_desktop_bg();
                draw_desktop_icons();
                for (int i = 0; i < num_windows; i++) {
                    draw_window_win(&windows[i]);
                }
                draw_taskbar();
            }
        }
        
        if (c >= '1' && c <= '4') {
            int sel = c - '1';
            if (sel == 0) {
                start_menu_open = false;
                run_calculator();
                draw_desktop_bg();
                draw_desktop_icons();
                for (int i = 0; i < num_windows; i++) {
                    draw_window_win(&windows[i]);
                }
                draw_taskbar();
            } else if (sel == 1) {
                start_menu_open = false;
                show_notepad();
                draw_desktop_bg();
                draw_desktop_icons();
                for (int i = 0; i < num_windows; i++) {
                    draw_window_win(&windows[i]);
                }
                draw_taskbar();
            } else if (sel == 2) {
                start_menu_open = false;
                show_about();
                draw_desktop_bg();
                draw_desktop_icons();
                for (int i = 0; i < num_windows; i++) {
                    draw_window_win(&windows[i]);
                }
                draw_taskbar();
            } else if (sel == 3) {
                return;
            }
        }
        
        if (c == 'c' || c == 'C') {
            start_menu_open = false;
            run_calculator();
            draw_desktop_bg();
            draw_desktop_icons();
            for (int i = 0; i < num_windows; i++) {
                draw_window_win(&windows[i]);
            }
            draw_taskbar();
        }
        
        if (c == 'n' || c == 'N') {
            start_menu_open = false;
            show_notepad();
            draw_desktop_bg();
            draw_desktop_icons();
            for (int i = 0; i < num_windows; i++) {
                draw_window_win(&windows[i]);
            }
            draw_taskbar();
        }
        
        if (k == 0x0F) {
            draw_calc_in_window();
        }
    }
}

extern "C" void kernel_main() {
    while ((inb(0x64) & 1) != 0) inb(0x60);
    
    init_keyboard();
    
    clear();
    logo();
    puts(VERSION);
    puts("\nType 'help' for commands.\n\n");

    char buf[64];
    int pos = 0;

    while (1) {
        puts("> ");
        
        pos = 0;
        while (pos < 63) {
            uint8_t k = wait_key();
            
            if ((k & 0x80) != 0) continue;
            
            char c = scancode_to_ascii(k);
            
            if (c == '\n') {
                buf[pos] = 0;
                putc('\n');
                break;
            }
            
            if (c == '\b') {
                if (pos > 0) {
                    pos--;
                    if (cx > 0) {
                        cx--;
                    } else if (cy > 0) {
                        cy--;
                        cx = VGA_WIDTH - 1;
                    }
                    vga[cy * VGA_WIDTH + cx] = 0x0F00 | ' ';
                    uint16_t pos2 = cy * VGA_WIDTH + cx;
                    outb(0x3D4, 14);
                    outb(0x3D5, pos2 >> 8);
                    outb(0x3D4, 15);
                    outb(0x3D5, pos2);
                }
                continue;
            }
            
            if (c != 0) {
                buf[pos++] = c;
                putc(c);
            }
        }
        buf[63] = 0;

        if (buf[0] == 0) continue;

        if (streq(buf, "help")) {
            help();
        }
        else if (streq(buf, "clear")) {
            clear();
        }
        else if (streq(buf, "ver")) {
            puts(VERSION);
            putc('\n');
        }
        else if (streq(buf, "reboot")) {
            reboot();
        }
        else if (streq(buf, "poweroff") || streq(buf, "shutdown")) {
            shutdown();
        }
        else if (streq(buf, "desktop")) {
            desktop();
        }
        else {
            puts("Unknown command: ");
            puts(buf);
            putc('\n');
        }
    }
}
