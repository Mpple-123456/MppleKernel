import os
import glob
import subprocess
import sys
import threading
import tkinter as tk
from tkinter import filedialog, scrolledtext, messagebox, font

class BuildUI(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("Mpple Kernel Maker")
        self.geometry("800x500")  # 加宽以容纳更多按钮
        self.qemu_process = None

        # 设置默认中文字体（如果系统中有文泉驿微米黑）
        try:
            default_font = font.nametofont("TkDefaultFont")
            default_font.configure(family="WenQuanYi Micro Hei", size=10)
        except:
            pass  # 如果字体不存在则忽略，使用系统默认

        # 工作目录选择
        frame_dir = tk.Frame(self)
        frame_dir.pack(pady=5, fill=tk.X)
        tk.Label(frame_dir, text="工作目录:").pack(side=tk.LEFT, padx=5)
        self.entry_dir = tk.Entry(frame_dir, width=50)
        self.entry_dir.pack(side=tk.LEFT, expand=True, fill=tk.X, padx=5)
        tk.Button(frame_dir, text="浏览", command=self.select_dir).pack(side=tk.LEFT, padx=5)

        # 工具路径配置（可选，留空则直接使用命令名）
        frame_tools = tk.Frame(self)
        frame_tools.pack(pady=5, fill=tk.X)
        tk.Label(frame_tools, text="编译器路径 (g++):").pack(side=tk.LEFT, padx=5)
        self.entry_gxx = tk.Entry(frame_tools, width=20)
        self.entry_gxx.pack(side=tk.LEFT, padx=2)
        self.entry_gxx.insert(0, "g++")
        tk.Label(frame_tools, text="链接器 (ld):").pack(side=tk.LEFT, padx=5)
        self.entry_ld = tk.Entry(frame_tools, width=20)
        self.entry_ld.pack(side=tk.LEFT, padx=2)
        self.entry_ld.insert(0, "ld")
        tk.Label(frame_tools, text="objcopy:").pack(side=tk.LEFT, padx=5)
        self.entry_objcopy = tk.Entry(frame_tools, width=20)
        self.entry_objcopy.pack(side=tk.LEFT, padx=2)
        self.entry_objcopy.insert(0, "objcopy")

        frame_tools2 = tk.Frame(self)
        frame_tools2.pack(pady=5, fill=tk.X)
        tk.Label(frame_tools2, text="NASM:").pack(side=tk.LEFT, padx=5)
        self.entry_nasm = tk.Entry(frame_tools2, width=20)
        self.entry_nasm.pack(side=tk.LEFT, padx=2)
        self.entry_nasm.insert(0, "nasm")
        tk.Label(frame_tools2, text="QEMU:").pack(side=tk.LEFT, padx=5)
        self.entry_qemu = tk.Entry(frame_tools2, width=20)
        self.entry_qemu.pack(side=tk.LEFT, padx=2)
        self.entry_qemu.insert(0, "qemu-system-i386")

        # 按钮区（第一行）
        frame_buttons = tk.Frame(self)
        frame_buttons.pack(pady=10)
        btn_kernel = tk.Button(frame_buttons, text="1. 编译内核", command=self.compile_kernel, width=14)
        btn_kernel.pack(side=tk.LEFT, padx=2)
        btn_link = tk.Button(frame_buttons, text="2. 链接内核", command=self.link_kernel, width=14)
        btn_link.pack(side=tk.LEFT, padx=2)
        btn_objcopy = tk.Button(frame_buttons, text="3. 生成二进制", command=self.objcopy_kernel, width=14)
        btn_objcopy.pack(side=tk.LEFT, padx=2)
        btn_makeimg = tk.Button(frame_buttons, text="4. 制作镜像", command=self.make_image, width=14)
        btn_makeimg.pack(side=tk.LEFT, padx=2)
        btn_run = tk.Button(frame_buttons, text="5. 运行 QEMU", command=self.run_qemu, width=14, bg="lightgreen")
        btn_run.pack(side=tk.LEFT, padx=2)
        # 新增清理按钮
        btn_clean = tk.Button(frame_buttons, text="6. 清理文件", command=self.clean_files, width=14, bg="lightcoral")
        btn_clean.pack(side=tk.LEFT, padx=2)

        # 按钮区（第二行，放置辅助功能）
        frame_buttons2 = tk.Frame(self)
        frame_buttons2.pack(pady=5)
        btn_all = tk.Button(frame_buttons2, text="全部执行 (1-4)", command=self.run_all, width=14, bg="lightblue")
        btn_all.pack(side=tk.LEFT, padx=2)
        btn_stop_qemu = tk.Button(frame_buttons2, text="停止 QEMU", command=self.stop_qemu, width=14, bg="orange")
        btn_stop_qemu.pack(side=tk.LEFT, padx=2)
        # 新增清除输出按钮
        btn_clear_log = tk.Button(frame_buttons2, text="清除输出", command=self.clear_log, width=14, bg="lightgray")
        btn_clear_log.pack(side=tk.LEFT, padx=2)

        # 输出文本框
        self.text_output = scrolledtext.ScrolledText(self, wrap=tk.WORD, height=15)
        self.text_output.pack(pady=5, padx=5, fill=tk.BOTH, expand=True)

        # 提示信息
        tk.Label(self, text="注意：工具链需已安装并加入 PATH，或填写完整路径；QEMU 启动后不会阻塞 UI，可手动停止。",
                 fg="gray", font=("Arial", 8)).pack(pady=2)

    def select_dir(self):
        directory = filedialog.askdirectory()
        if directory:
            self.entry_dir.delete(0, tk.END)
            self.entry_dir.insert(0, directory)

    def log(self, message, color="black"):
        """向文本框添加带颜色的消息（在主线程中调用）"""
        self.text_output.insert(tk.END, message + "\n")
        self.text_output.see(tk.END)
        self.update_idletasks()

    def clear_log(self):
        """清空输出文本框"""
        self.text_output.delete(1.0, tk.END)

    def run_cmd(self, cmd, cwd=None, capture=True):
        """
        运行命令，返回 (success, output)
        如果 capture=True，等待命令结束并捕获输出；否则只启动进程不等待。
        """
        if cwd is None:
            cwd = self.entry_dir.get() or os.getcwd()
        self.log(f"执行: {' '.join(cmd)}", "blue")
        try:
            if capture:
                result = subprocess.run(cmd, cwd=cwd, capture_output=True, text=True, check=False)
                out = result.stdout + result.stderr
                if result.returncode != 0:
                    self.log(f"失败 (返回码 {result.returncode}):", "red")
                    self.log(out, "red")
                    return False, out
                else:
                    self.log("成功", "green")
                    self.log(out, "black")
                    return True, out
            else:
                self.qemu_process = subprocess.Popen(cmd, cwd=cwd)
                self.log(f"QEMU 已启动 (PID: {self.qemu_process.pid})", "green")
                return True, ""
        except FileNotFoundError as e:
            msg = f"命令未找到: {e}. 请检查工具路径或是否已安装。"
            self.log(msg, "red")
            return False, msg

    def compile_kernel(self):
        gxx = self.entry_gxx.get().strip() or "g++"
        cmd = [gxx, "-m32", "-ffreestanding", "-nostdlib", "-fno-builtin",
               "-fno-exceptions", "-fno-rtti", "-O2", "-c", "kernel.cpp",
               "-o", "kernel.o", "-std=c++11"]
        self.run_cmd(cmd)

    def link_kernel(self):
        ld = self.entry_ld.get().strip() or "ld"
        cmd = [ld, "-m", "elf_i386", "-T", "linker.ld", "-o", "kernel.elf", "kernel.o"]
        self.run_cmd(cmd)

    def objcopy_kernel(self):
        objcopy = self.entry_objcopy.get().strip() or "objcopy"
        cmd = [objcopy, "-O", "binary", "kernel.elf", "kernel.bin"]
        self.run_cmd(cmd)

    def make_image(self):
        nasm = self.entry_nasm.get().strip() or "nasm"
        # 生成 boot.bin
        cmd1 = [nasm, "-f", "bin", "boot.asm", "-o", "boot.bin"]
        ok, _ = self.run_cmd(cmd1)
        if not ok:
            return
        # 创建空白镜像（1.44MB）
        cwd = self.entry_dir.get() or os.getcwd()
        img_path = os.path.join(cwd, "MppleKernel.img")
        try:
            with open(img_path, "wb") as f:
                f.seek(1474560 - 1)
                f.write(b'\x00')
            self.log("创建空白镜像 MppleKernel.img (1.44MB)", "green")
        except Exception as e:
            self.log(f"创建镜像失败: {e}", "red")
            return

        # 写入 boot.bin 到第一个扇区
        boot_path = os.path.join(cwd, "boot.bin")
        try:
            with open(img_path, "r+b") as img:
                with open(boot_path, "rb") as boot:
                    img.write(boot.read())
            self.log("写入 boot.bin 到扇区0", "green")
        except Exception as e:
            self.log(f"写入 boot.bin 失败: {e}", "red")
            return

        # 写入 kernel.bin 从第二个扇区开始 (seek=512)
        kernel_path = os.path.join(cwd, "kernel.bin")
        try:
            with open(img_path, "r+b") as img:
                img.seek(512)
                with open(kernel_path, "rb") as ker:
                    img.write(ker.read())
            self.log("写入 kernel.bin 到扇区1及之后", "green")
        except Exception as e:
            self.log(f"写入 kernel.bin 失败: {e}", "red")
            return

        self.log("Done!", "green")

    def run_qemu(self):
        qemu = self.entry_qemu.get().strip() or "qemu-system-i386"
        cwd = self.entry_dir.get() or os.getcwd()
        img_path = os.path.join(cwd, "MppleKernel.img")
        if not os.path.exists(img_path):
            self.log("镜像文件 MppleKernel.img 不存在，请先制作镜像", "red")
            return
        cmd = [qemu, "-drive", "format=raw,file=MppleKernel.img", "-fda", "MppleKernel.img", "-snapshot"]
        self.run_cmd(cmd, capture=False)

    def stop_qemu(self):
        if self.qemu_process and self.qemu_process.poll() is None:
            self.qemu_process.terminate()
            self.log("已发送终止信号给 QEMU", "orange")
        else:
            self.log("没有正在运行的 QEMU 进程", "gray")

    def run_all(self):
        """按顺序执行编译、链接、二进制生成、镜像制作（不自动运行 QEMU）"""
        self.compile_kernel()
        self.link_kernel()
        self.objcopy_kernel()
        self.make_image()

    def clean_files(self):
        """清理生成的文件：*.o, *.elf, *.bin, *.img"""
        cwd = self.entry_dir.get() or os.getcwd()
        patterns = ['*.o', '*.elf', '*.bin', 'MppleKernel.img']
        deleted = []
        failed = []
        for pattern in patterns:
            for filepath in glob.glob(os.path.join(cwd, pattern)):
                try:
                    os.remove(filepath)
                    deleted.append(os.path.basename(filepath))
                except Exception as e:
                    failed.append(f"{os.path.basename(filepath)} ({e})")
        if deleted:
            self.log("已删除文件: " + ", ".join(deleted), "green")
        if failed:
            self.log("删除失败: " + ", ".join(failed), "red")
        if not deleted and not failed:
            self.log("没有找到要清理的文件", "gray")
        # 如果 QEMU 正在运行，提示可能无法删除 img
        if self.qemu_process and self.qemu_process.poll() is None:
            self.log("注意: QEMU 正在运行，可能无法删除镜像文件，请先停止 QEMU。", "orange")

if __name__ == "__main__":
    app = BuildUI()
    app.mainloop()