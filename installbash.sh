#!/bin/bash

# MppleOS 依赖安装脚本
# 支持 Debian/Ubuntu (apt), Fedora/RHEL (dnf/yum), Arch (pacman), openSUSE (zypper), Alpine (apk)

set -e  # 遇到错误立即退出

# 检查是否以 root 权限运行
if [ "$EUID" -ne 0 ]; then
    echo "请使用 sudo 或以 root 身份运行此脚本。"
    exit 1
fi

echo "========================================"
echo "    MppleOS 依赖安装脚本"
echo "========================================"

# 检测发行版
detect_distro() {
    if [ -f /etc/os-release ]; then
        . /etc/os-release
        DISTRO_ID=$ID
        DISTRO_LIKE=$ID_LIKE
    else
        echo "无法检测 Linux 发行版。"
        exit 1
    fi
}

# 安装依赖
install_deps() {
    case $DISTRO_ID in
        ubuntu|debian|linuxmint|pop|raspbian)
            echo "检测到 Debian/Ubuntu 系发行版，使用 apt 安装..."
            apt update
            apt install -y build-essential g++ nasm qemu-system-x86 qemu-utils \
                           python3 python3-tk
            ;;
        fedora|rhel|centos)
            if command -v dnf >/dev/null 2>&1; then
                echo "检测到 Fedora/RHEL 系发行版，使用 dnf 安装..."
                dnf install -y gcc-c++ nasm qemu-system-x86 qemu-img \
                               python3 python3-tkinter
            else
                echo "检测到 CentOS 7 或更早版本，使用 yum 安装..."
                yum install -y gcc-c++ nasm qemu-system-x86 qemu-img \
                               python3 python3-tkinter
            fi
            ;;
        arch|manjaro|endeavouros)
            echo "检测到 Arch 系发行版，使用 pacman 安装..."
            pacman -Syu --noconfirm base-devel gcc nasm qemu-system-x86 qemu-img \
                           python python-tk
            ;;
        opensuse*|suse)
            echo "检测到 openSUSE 系发行版，使用 zypper 安装..."
            zypper refresh
            zypper install -y gcc-c++ nasm qemu-x86 qemu-tools \
                           python3 python3-tk
            ;;
        alpine)
            echo "检测到 Alpine Linux，使用 apk 安装..."
            apk add build-base g++ nasm qemu-system-x86_64 qemu-img \
                    python3 py3-tkinter
            ;;
        *)
            echo "未知发行版：$DISTRO_ID，请手动安装以下软件："
            echo "  - g++ (或 clang++)"
            echo "  - ld (binutils)"
            echo "  - objcopy (binutils)"
            echo "  - nasm"
            echo "  - qemu-system-i386"
            echo "  - python3"
            echo "  - python3-tk (Tkinter)"
            exit 1
            ;;
    esac
}

# 检查关键工具是否安装成功
check_tools() {
    local MISSING=0
    echo ""
    echo "检查工具安装情况："
    for tool in g++ ld objcopy nasm qemu-system-i386 python3; do
        if command -v $tool >/dev/null 2>&1; then
            echo "  [✔] $tool"
        else
            echo "  [✘] $tool"
            MISSING=1
        fi
    done
    # 额外检查 Tkinter
    if python3 -c "import tkinter" 2>/dev/null; then
        echo "  [✔] Tkinter (Python GUI 库)"
    else
        echo "  [✘] Tkinter (Python GUI 库) - 请确保已安装 python3-tk"
        MISSING=1
    fi
    if [ $MISSING -eq 1 ]; then
        echo "部分工具未找到，请手动检查安装。"
        exit 1
    else
        echo "所有工具已正确安装！"
    fi
}

# 主流程
detect_distro
install_deps
check_tools

echo ""
echo "依赖安装完成！现在可以运行 python3 make.py 构建 MppleOS 了。"