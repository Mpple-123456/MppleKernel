#!/bin/bash

# MppleOS 依赖安装脚本
# 支持 Debian/Ubuntu (apt), Fedora/RHEL (dnf/yum), Arch (pacman), openSUSE (zypper), Alpine (apk)

set -e

if [ "$EUID" -ne 0 ]; then
    echo "请使用 sudo 或以 root 身份运行此脚本。"
    exit 1
fi

echo "========================================"
echo "    MppleOS 依赖安装脚本"
echo "========================================"

detect_distro() {
    if [ -f /etc/os-release ]; then
        . /etc/os-release
        DISTRO_ID=$ID
    else
        echo "无法检测 Linux 发行版。"
        exit 1
    fi
}

install_deps() {
    case $DISTRO_ID in
        ubuntu|debian|linuxmint|pop|raspbian)
            echo "检测到 Debian/Ubuntu 系发行版，使用 apt 安装..."
            apt update
            apt install -y build-essential g++ nasm qemu-system-x86 qemu-utils \
                           python3 python3-tk grub-common grub-pc-bin xorriso
            ;;
        fedora|rhel|centos)
            if command -v dnf >/dev/null 2>&1; then
                dnf install -y gcc-c++ nasm qemu-system-x86 qemu-img \
                               python3 python3-tkinter grub2-common grub2-pc xorriso
            else
                yum install -y gcc-c++ nasm qemu-system-x86 qemu-img \
                               python3 python3-tkinter grub2-common grub2-pc xorriso
            fi
            ;;
        arch|manjaro|endeavouros)
            pacman -Syu --noconfirm base-devel gcc nasm qemu-system-x86 qemu-img \
                           python python-tk grub libisoburn
            ;;
        opensuse*|suse)
            zypper refresh
            zypper install -y gcc-c++ nasm qemu-x86 qemu-tools \
                           python3 python3-tk grub2 xorriso
            ;;
        alpine)
            apk add build-base g++ nasm qemu-system-x86_64 qemu-img \
                    python3 py3-tkinter grub xorriso
            ;;
        *)
            echo "未知发行版，请手动安装以下软件："
            echo "  - g++, ld, objcopy (binutils)"
            echo "  - nasm"
            echo "  - qemu-system-i386"
            echo "  - python3, python3-tk"
            echo "  - grub-common, grub-pc-bin, xorriso"
            exit 1
            ;;
    esac
}

check_tools() {
    local MISSING=0
    echo ""
    echo "检查工具安装情况："
    for tool in g++ ld objcopy nasm qemu-system-i386 python3 grub-mkrescue xorriso; do
        if command -v $tool >/dev/null 2>&1; then
            echo "  [✔] $tool"
        else
            echo "  [✘] $tool"
            MISSING=1
        fi
    done
    if python3 -c "import tkinter" 2>/dev/null; then
        echo "  [✔] Tkinter"
    else
        echo "  [✘] Tkinter"
        MISSING=1
    fi
    if [ $MISSING -eq 1 ]; then
        echo "部分工具未找到，请手动检查安装。"
        exit 1
    else
        echo "所有工具已正确安装！"
    fi
}

detect_distro
install_deps
check_tools
echo ""
echo "依赖安装完成！现在可以运行 python3 make.py 或 ./build.sh 构建内核。"