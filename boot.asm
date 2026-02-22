[bits 16]
[org 0x7c00]

start:
    ; 初始化段寄存器
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00
    sti

    ; 保存引导驱动器号
    mov [boot_drive], dl

    ; 显示加载信息
    mov si, loading_msg
    call print_string

    ; 重置磁盘系统
    mov ah, 0
    int 0x13

    ; 加载内核到内存 0x1000:0x0000
    mov ax, 0x1000
    mov es, ax
    xor bx, bx          ; ES:BX = 0x1000:0x0000
    
    mov ah, 2           ; 功能号：读扇区
    mov al, 40          ; 读取 20 个扇区
    mov ch, 0           ; 柱面
    mov cl, 2           ; 起始扇区（2，因为第一个是引导扇区）
    mov dh, 0           ; 磁头
    mov dl, [boot_drive]; 使用保存的驱动器号
    int 0x13
    jc load_error

    ; 显示成功信息
    mov si, success_msg
    call print_string

    ; 切换到保护模式
    cli
    lgdt [gdt_desc]

    ; 开启 A20 线
    in al, 0x92
    or al, 2
    out 0x92, al

    mov eax, cr0
    or eax, 1
    mov cr0, eax

    jmp 0x08:protected_mode

print_string:
    lodsb
    or al, al
    jz .done
    mov ah, 0x0e
    int 0x10
    jmp print_string
.done:
    ret

load_error:
    mov si, error_msg
    call print_string
.hang:
    jmp .hang

[bits 32]
protected_mode:
    ; 设置段寄存器
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000

    ; 清屏
    mov edi, 0xB8000
    mov ecx, 80*25
    mov ax, 0x0720
    rep stosw

    ; 显示保护模式信息
    mov edi, 0xB8000
    mov esi, protected_msg
    call print_string_32

    ; 跳转到内核入口（0x10000）
    call 0x10000

    ; 如果内核返回，则停机
    hlt
    jmp $

print_string_32:
    mov ah, 0x07
.next:
    lodsb
    or al, al
    jz .done
    mov [edi], al
    inc edi
    mov [edi], ah
    inc edi
    jmp .next
.done:
    ret

; 数据
boot_drive: db 0
loading_msg db "Loading MppleKernel...", 13, 10, 0
success_msg db "Kernel loaded successfully!", 13, 10, 0
error_msg db "Failed to load kernel!", 13, 10, 0
protected_msg db "Protected mode entered, starting kernel...", 0

; GDT
gdt:
    dq 0                ; 空描述符
    ; 代码段描述符
    dw 0xFFFF
    dw 0
    db 0
    db 0x9A
    db 0xCF
    db 0
    ; 数据段描述符
    dw 0xFFFF
    dw 0
    db 0
    db 0x92
    db 0xCF
    db 0
gdt_end:

gdt_desc:
    dw gdt_end - gdt - 1
    dd gdt

times 510-($-$$) db 0
dw 0xAA55