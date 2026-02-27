# MppleKernel
MppleOS

MppleOS 是一个从零开始编写的简易 x86 操作系统内核，旨在提供轻量级、可学习的操作系统实现。它支持命令行交互、文本模式桌面，并内置了错误处理机制，适合作为操作系统开发的入门参考项目。

✨ 特性
✅ 引导程序：从实模式切换到保护模式，加载内核到 0x10000

✅ VGA 文本输出：80x25 字符模式，支持硬件光标

✅ PS/2 键盘驱动：支持 Shift、Caps Lock，屏幕右上角显示状态

✅ 命令行 Shell：内置 help, clear, echo, ver, poweroff, reboot 等命令

✅ 文本桌面环境：通过 desktop 命令进入，可使用数字键操作

✅ 错误处理：KERNEL PANIC 机制，发生严重错误时显示信息并重启

✅ 构建工具：图形化 make.py 脚本，一键编译、链接、制作镜像

✅ 物理机支持：提供 install.sh 将系统写入 USB 设备

🚀 快速开始
环境要求
Linux 环境（推荐 Ubuntu/Debian）或 WSL

工具链：g++, ld, objcopy, nasm

模拟器：qemu-system-i386（用于测试）

Python 3 + Tkinter（运行 make.py）

一键安装所有依赖（支持多种发行版）：

bash
sudo ./installbash.sh
构建镜像
使用图形化构建工具：

bash
python3 make.py
点击“全部执行 (1-4)”即可生成 MppleKernel.img。

或手动执行命令：

bash
g++ -m32 -ffreestanding -nostdlib -fno-builtin -fno-exceptions -fno-rtti -O2 -c kernel.cpp -o kernel.o -std=c++11
ld -m elf_i386 -T linker.ld -o kernel.elf kernel.o
objcopy -O binary kernel.elf kernel.bin
nasm -f bin boot.asm -o boot.bin
dd if=/dev/zero of=MppleKernel.img bs=512 count=2880
dd if=boot.bin of=MppleKernel.img conv=notrunc
dd if=kernel.bin of=MppleKernel.img seek=1 conv=notrunc
在 QEMU 中运行
bash
qemu-system-i386 -drive format=raw,file=MppleKernel.img -fda MppleKernel.img -snapshot
安装到 USB 设备
警告：此操作会销毁设备上的所有数据！

bash
sudo ./install.sh
按照提示选择目标设备（如 /dev/sdb），输入 YES 确认。

📖 使用方法
启动后进入命令行，提示符为 >。支持以下命令：

命令	说明
help	显示帮助信息
clear	清屏
echo <text>	回显文本
ver	显示内核版本
poweroff / shutdown	关闭系统
reboot	重启系统
RESETKB	重置键盘状态
run <progname>	运行外部程序（预留）
desktop	进入桌面环境
在桌面环境中，按数字键 1、2、3 分别对应返回命令行、关机、重启。

键盘使用技巧：

退格键：删除前一个字符

Caps Lock：切换大小写，右上角显示 CAPS

Shift：临时切换大小写或输入符号

📁 项目结构
text
.
├── boot.asm          # 引导程序（实模式 → 保护模式）
├── kernel.cpp        # 内核主代码（所有功能实现）
├── linker.ld         # 链接脚本（定义内核内存布局）
├── make.py           # 图形化构建工具
├── install.sh        # 写入 USB 设备的脚本
├── installbash.sh    # 自动安装依赖的脚本
└── README.md         # 本文件

Happy Hacking! 🖥️
