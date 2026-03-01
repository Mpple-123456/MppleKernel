# Mpple Kernel

Mpple Kernel 是一个从零开始编写的简易 x86 操作系统内核，旨在提供轻量级、可学习的操作系统实现。它支持命令行交互、文本模式桌面，并内置了错误处理机制。内核可通过传统软盘镜像或 GRUB ISO 引导，适合作为操作系统开发的入门参考项目。

---

## ✨ 特性

- ✅ **双引导支持**：可通过自定义引导程序（`boot.asm`）或 GRUB（Multiboot）启动
- ✅ **VGA 文本输出**：80x25 字符模式，支持硬件光标
- ✅ **PS/2 键盘驱动**：支持 Shift、Caps Lock，屏幕右上角显示状态
- ✅ **命令行 Shell**：内置 `help`, `clear`, `echo`, `ver`, `poweroff`, `reboot`, `desktop` 等命令
- ✅ **文本桌面环境**：通过 `desktop` 命令进入，使用数字键操作
- ✅ **错误处理**：`KERNEL PANIC` 机制，发生严重错误时显示信息并重启
- ✅ **构建工具**：提供命令行脚本 `build.sh` 和图形化工具 `make.py`
- ✅ **物理机支持**：提供 `install.sh` 将内核写入 USB 设备

---

## 📁 项目结构
.
├── boot/ # 引导相关文件
│ ├── boot.asm # 传统引导程序
│ └── grub.cfg # GRUB 配置文件
├── kernel/ # 内核源码
│ ├── kernel.cpp # 主内核代码
│ └── linker.ld # 链接脚本
├── build/ # 构建脚本（所有命令在此运行）
│ ├── build.sh # 命令行构建脚本
│ └── make.py # 图形化构建工具
├── install.sh # 将系统写入 USB 设备的脚本
├── installbash.sh # 自动安装依赖的脚本
└── README.md # 本文件

text

---

## 🚀 快速开始

### 环境要求

- Linux 环境（推荐 Ubuntu/Debian）或 WSL
- 工具链：`g++`, `ld`, `objcopy`, `nasm`
- 模拟器：`qemu-system-i386`（用于测试）
- Python 3 + Tkinter（运行 `make.py` 需要）

一键安装所有依赖（支持多种发行版）：
```bash
sudo ./installbash.sh
构建内核
所有构建命令必须在 build/ 目录下执行：

bash
cd build
使用命令行脚本
bash
./build.sh --grub -a   # 编译内核并生成软盘镜像和 GRUB ISO
可用选项：

-c：仅编译内核

-l：仅链接内核

-b：仅生成二进制

-f：仅制作软盘镜像

-i：仅制作 ISO 镜像

-r：运行 QEMU

--grub：编译 GRUB 版本（启用 Multiboot）

-C：清理生成的文件

使用图形化工具
    python3 make.py
勾选“编译 GRUB 版本”，然后点击相应按钮执行步骤。

在 QEMU 中运行
    ./build.sh -r
将自动优先使用 ISO 镜像启动；若无 ISO 则使用软盘镜像。

安装到 USB 设备
警告：此操作会销毁设备上的所有数据！

    sudo ./install.sh
按照提示选择目标设备（如 /dev/sdb），输入 YES 确认后写入。

📖 使用指南
启动后进入命令行，提示符为 >。支持以下命令：

命令	说明
help	显示帮助信息
clear	清屏
echo <text>	回显文本
ver	显示内核版本
poweroff / shutdown	关闭系统
reboot	重启系统
RESETKB	重置键盘状态
desktop	进入桌面环境
桌面环境中，按数字键 1（返回命令行）、2（关机）、3（重启）选择功能。

键盘技巧：

退格键：删除前一个字符

Caps Lock：切换大小写，右上角显示 CAPS

Shift：临时切换大小写或输入符号

Happy Hacking! 🖥️

