#!/bin/bash

# Mpple Kernel 命令行构建脚本
# 必须在 build/ 目录内运行
# 用法: ./build.sh [选项]

set -e

GXX="g++"
LD="ld"
OBJCOPY="objcopy"
NASM="nasm"
QEMU="qemu-system-i386"
GRUB=0
UEFI=0
ACTION="all"
KERNEL_ELF="kernel.elf"
KERNEL_BIN="kernel.bin"
BOOT_BIN="boot.bin"
FLOPPY_IMG="../MppleKernel.img"
ISO_IMG="../MppleKernel.iso"

usage() {
    echo "Mpple Kernel 构建脚本"
    echo "用法: $0 [选项]"
    echo "选项:"
    echo "  -h, --help      显示帮助"
    echo "  -c, --compile   仅编译内核"
    echo "  -l, --link      仅链接内核"
    echo "  -b, --binary    仅生成二进制"
    echo "  -f, --floppy    制作软盘镜像"
    echo "  -i, --iso       制作 ISO 镜像"
    echo "  -r, --run       运行 QEMU"
    echo "  -a, --all       执行全部步骤 (默认)"
    echo "  --grub          编译 GRUB 版本 (定义 -DGRUB)"
    echo "  --uefi          制作 UEFI 可启动 ISO (需要 grub-efi & mtools)"
    echo "  -C, --clean     清理生成文件"
    echo "  --gcc=PATH      指定 g++"
    echo "  --ld=PATH       指定 ld"
    echo "  --objcopy=PATH  指定 objcopy"
    echo "  --nasm=PATH     指定 nasm"
    echo "  --qemu=PATH     指定 qemu"
    exit 0
}

check_cmd() {
    if ! command -v "$1" &> /dev/null; then
        echo "错误: 未找到命令 '$1'"
        exit 1
    fi
}

clean() {
    echo "清理文件..."
    rm -f kernel.o kernel.elf kernel.bin boot.bin
    rm -f "$FLOPPY_IMG" "$ISO_IMG"
    rm -rf iso
    echo "完成"
}

compile() {
    echo "编译内核..."
    check_cmd "$GXX"
    CFLAGS="-m32 -ffreestanding -nostdlib -fno-builtin -fno-exceptions -fno-rtti -O2 -c ../kernel/kernel.cpp -o kernel.o -std=c++11"
    [ $GRUB -eq 1 ] && CFLAGS="$CFLAGS -DGRUB" && echo "GRUB 模式"
    $GXX $CFLAGS
    echo "完成: kernel.o"
}

link() {
    echo "链接内核..."
    check_cmd "$LD"
    $LD -m elf_i386 -T ../kernel/linker.ld -o "$KERNEL_ELF" kernel.o
    echo "完成: $KERNEL_ELF"
}

binary() {
    echo "生成二进制..."
    check_cmd "$OBJCOPY"
    $OBJCOPY -O binary "$KERNEL_ELF" "$KERNEL_BIN"
    echo "完成: $KERNEL_BIN"
}

floppy() {
    echo "制作软盘镜像..."
    check_cmd "$NASM"
    nasm -f bin ../boot/boot.asm -o "$BOOT_BIN"
    dd if=/dev/zero of="$FLOPPY_IMG" bs=512 count=2880 2>/dev/null
    dd if="$BOOT_BIN" of="$FLOPPY_IMG" conv=notrunc 2>/dev/null
    dd if="$KERNEL_BIN" of="$FLOPPY_IMG" seek=1 conv=notrunc 2>/dev/null
    echo "完成: $FLOPPY_IMG"
}

iso() {
    echo "制作 ISO 镜像..."
    check_cmd "grub-mkrescue"
    check_cmd "xorriso"

    if [ $UEFI -eq 1 ]; then
        echo "UEFI 模式已启用，将生成混合 ISO"
        check_cmd "mformat"  # 需要 mtools
    fi

    # 创建 ISO 目录结构
    mkdir -p iso/boot/grub
    cp "$KERNEL_ELF" iso/boot/

    cat > iso/boot/grub/grub.cfg << EOF
set timeout=5
set default=0
menuentry "Mpple Kernel" {
    multiboot /boot/kernel.elf
    boot
}
EOF

    # grub-mkrescue 会自动处理 BIOS 和 UEFI（如果安装了相应模块）
    grub-mkrescue -o "$ISO_IMG" iso

    rm -rf iso
    echo "完成: $ISO_IMG"
}

run_qemu() {
    check_cmd "$QEMU"
    if [ -f "$ISO_IMG" ]; then
        if [ $UEFI -eq 1 ]; then
            if [ -f /usr/share/ovmf/OVMF.fd ]; then
                echo "启动 QEMU (UEFI 模式)..."
                $QEMU -bios /usr/share/ovmf/OVMF.fd -cdrom "$ISO_IMG" -boot d -snapshot
            else
                echo "警告: OVMF 未找到，使用 BIOS 模式启动"
                $QEMU -cdrom "$ISO_IMG" -boot d -snapshot
            fi
        else
            echo "启动 QEMU (BIOS 模式)..."
            $QEMU -cdrom "$ISO_IMG" -boot d -snapshot
        fi
    elif [ -f "$FLOPPY_IMG" ]; then
        echo "启动 QEMU (软盘)..."
        $QEMU -drive format=raw,file="$FLOPPY_IMG" -fda "$FLOPPY_IMG" -snapshot
    else
        echo "未找到镜像文件"
        exit 1
    fi
}

# 解析参数
while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help) usage ;;
        -c|--compile) ACTION="compile" ;;
        -l|--link) ACTION="link" ;;
        -b|--binary) ACTION="binary" ;;
        -f|--floppy) ACTION="floppy" ;;
        -i|--iso) ACTION="iso" ;;
        -r|--run) ACTION="run" ;;
        -a|--all) ACTION="all" ;;
        --grub) GRUB=1 ;;
        --uefi) UEFI=1 ;;
        -C|--clean) ACTION="clean" ;;
        --gcc=*) GXX="${1#*=}" ;;
        --ld=*) LD="${1#*=}" ;;
        --objcopy=*) OBJCOPY="${1#*=}" ;;
        --nasm=*) NASM="${1#*=}" ;;
        --qemu=*) QEMU="${1#*=}" ;;
        *) echo "未知选项: $1"; usage ;;
    esac
    shift
done

# 如果制作 ISO 且启用了 UEFI，自动启用 GRUB 模式
if [ $UEFI -eq 1 ] && [[ "$ACTION" == "iso" || "$ACTION" == "all" ]]; then
    GRUB=1
fi

case $ACTION in
    clean) clean ;;
    compile) compile ;;
    link) link ;;
    binary) binary ;;
    floppy) floppy ;;
    iso) iso ;;
    run) run_qemu ;;
    all)
        compile
        link
        binary
        floppy
        iso
        echo "所有步骤完成！"
        ;;
esac