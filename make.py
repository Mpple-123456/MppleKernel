import os
import glob
import subprocess
import shutil
import tempfile
import tkinter as tk
from tkinter import filedialog, scrolledtext, font, Checkbutton, IntVar

class BuildUI(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("Mpple Kernel Maker")
        self.geometry("850x550")
        self.qemu_process = None

        try:
            default_font = font.nametofont("TkDefaultFont")
            default_font.configure(family="WenQuanYi Micro Hei", size=10)
        except:
            pass

        # 工作目录
        frame_dir = tk.Frame(self)
        frame_dir.pack(pady=5, fill=tk.X)
        tk.Label(frame_dir, text="工作目录:").pack(side=tk.LEFT, padx=5)
        self.entry_dir = tk.Entry(frame_dir, width=50)
        self.entry_dir.pack(side=tk.LEFT, expand=True, fill=tk.X, padx=5)
        tk.Button(frame_dir, text="浏览", command=self.select_dir).pack(side=tk.LEFT, padx=5)

        # 工具路径
        frame_tools = tk.Frame(self)
        frame_tools.pack(pady=5, fill=tk.X)
        tk.Label(frame_tools, text="g++:").pack(side=tk.LEFT, padx=5)
        self.entry_gxx = tk.Entry(frame_tools, width=12)
        self.entry_gxx.pack(side=tk.LEFT, padx=2)
        self.entry_gxx.insert(0, "g++")
        tk.Label(frame_tools, text="ld:").pack(side=tk.LEFT, padx=5)
        self.entry_ld = tk.Entry(frame_tools, width=12)
        self.entry_ld.pack(side=tk.LEFT, padx=2)
        self.entry_ld.insert(0, "ld")
        tk.Label(frame_tools, text="objcopy:").pack(side=tk.LEFT, padx=5)
        self.entry_objcopy = tk.Entry(frame_tools, width=12)
        self.entry_objcopy.pack(side=tk.LEFT, padx=2)
        self.entry_objcopy.insert(0, "objcopy")
        tk.Label(frame_tools, text="nasm:").pack(side=tk.LEFT, padx=5)
        self.entry_nasm = tk.Entry(frame_tools, width=12)
        self.entry_nasm.pack(side=tk.LEFT, padx=2)
        self.entry_nasm.insert(0, "nasm")
        tk.Label(frame_tools, text="qemu:").pack(side=tk.LEFT, padx=5)
        self.entry_qemu = tk.Entry(frame_tools, width=12)
        self.entry_qemu.pack(side=tk.LEFT, padx=2)
        self.entry_qemu.insert(0, "qemu-system-i386")

        # 选项区域
        frame_options = tk.Frame(self)
        frame_options.pack(pady=5, fill=tk.X)
        self.grub_var = IntVar()
        tk.Checkbutton(frame_options, text="编译 GRUB 版本 (-DGRUB)", variable=self.grub_var).pack(side=tk.LEFT, padx=10)

        # 按钮区第一行
        frame_buttons = tk.Frame(self)
        frame_buttons.pack(pady=10)
        btn_kernel = tk.Button(frame_buttons, text="1. 编译内核", command=self.compile_kernel, width=14)
        btn_kernel.pack(side=tk.LEFT, padx=2)
        btn_link = tk.Button(frame_buttons, text="2. 链接内核", command=self.link_kernel, width=14)
        btn_link.pack(side=tk.LEFT, padx=2)
        btn_bin = tk.Button(frame_buttons, text="3. 生成二进制", command=self.objcopy_kernel, width=14)
        btn_bin.pack(side=tk.LEFT, padx=2)

        # 第二行
        frame_buttons2 = tk.Frame(self)
        frame_buttons2.pack(pady=5)
        btn_floppy = tk.Button(frame_buttons2, text="制作软盘镜像", command=self.make_floppy_image, width=16, bg="lightblue")
        btn_floppy.pack(side=tk.LEFT, padx=2)
        btn_iso = tk.Button(frame_buttons2, text="制作 ISO (GRUB)", command=self.make_iso, width=16, bg="lightgreen")
        btn_iso.pack(side=tk.LEFT, padx=2)
        btn_run = tk.Button(frame_buttons2, text="运行 QEMU", command=self.run_qemu, width=14, bg="lightgreen")
        btn_run.pack(side=tk.LEFT, padx=2)
        btn_stop = tk.Button(frame_buttons2, text="停止 QEMU", command=self.stop_qemu, width=14, bg="orange")
        btn_stop.pack(side=tk.LEFT, padx=2)

        # 第三行
        frame_buttons3 = tk.Frame(self)
        frame_buttons3.pack(pady=5)
        btn_all = tk.Button(frame_buttons3, text="全部执行 (1-3)", command=self.run_all, width=14, bg="lightblue")
        btn_all.pack(side=tk.LEFT, padx=2)
        btn_clean = tk.Button(frame_buttons3, text="清理文件", command=self.clean_files, width=14, bg="lightcoral")
        btn_clean.pack(side=tk.LEFT, padx=2)
        btn_clear_log = tk.Button(frame_buttons3, text="清除输出", command=self.clear_log, width=14, bg="lightgray")
        btn_clear_log.pack(side=tk.LEFT, padx=2)

        self.text_output = scrolledtext.ScrolledText(self, wrap=tk.WORD, height=15)
        self.text_output.pack(pady=5, padx=5, fill=tk.BOTH, expand=True)

        tk.Label(self, text="注意：制作 ISO 需要安装 grub-common 和 xorriso", fg="gray").pack(pady=2)

    def select_dir(self):
        d = filedialog.askdirectory()
        if d:
            self.entry_dir.delete(0, tk.END)
            self.entry_dir.insert(0, d)

    def log(self, msg, color="black"):
        self.text_output.insert(tk.END, msg + "\n")
        self.text_output.see(tk.END)
        self.update_idletasks()

    def clear_log(self):
        self.text_output.delete(1.0, tk.END)

    def run_cmd(self, cmd, cwd=None, capture=True):
        if cwd is None:
            cwd = self.entry_dir.get() or os.getcwd()
        self.log(f"执行: {' '.join(cmd)}", "blue")
        try:
            if capture:
                r = subprocess.run(cmd, cwd=cwd, capture_output=True, text=True)
                out = r.stdout + r.stderr
                if r.returncode != 0:
                    self.log(f"失败 (返回码 {r.returncode}):", "red")
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
            self.log(f"命令未找到: {e}", "red")
            return False, ""

    def compile_kernel(self):
        gxx = self.entry_gxx.get().strip() or "g++"
        cmd = [gxx, "-m32", "-ffreestanding", "-nostdlib", "-fno-builtin",
               "-fno-exceptions", "-fno-rtti", "-O2", "-c", "kernel.cpp",
               "-o", "kernel.o", "-std=c++11"]
        if self.grub_var.get():
            cmd.append("-DGRUB")
            self.log("GRUB 模式已启用", "orange")
        self.run_cmd(cmd)

    def link_kernel(self):
        ld = self.entry_ld.get().strip() or "ld"
        cmd = [ld, "-m", "elf_i386", "-T", "linker.ld", "-o", "kernel.elf", "kernel.o"]
        self.run_cmd(cmd)

    def objcopy_kernel(self):
        objcopy = self.entry_objcopy.get().strip() or "objcopy"
        cmd = [objcopy, "-O", "binary", "kernel.elf", "kernel.bin"]
        self.run_cmd(cmd)

    def make_floppy_image(self):
        nasm = self.entry_nasm.get().strip() or "nasm"
        r, _ = self.run_cmd([nasm, "-f", "bin", "boot.asm", "-o", "boot.bin"])
        if not r:
            return
        cwd = self.entry_dir.get() or os.getcwd()
        img = os.path.join(cwd, "MppleKernel.img")
        try:
            with open(img, "wb") as f:
                f.seek(1474560 - 1)
                f.write(b'\x00')
            self.log("创建空白镜像 MppleKernel.img", "green")
        except Exception as e:
            self.log(f"创建失败: {e}", "red")
            return
        try:
            with open(img, "r+b") as f:
                with open("boot.bin", "rb") as b:
                    f.write(b.read())
                f.seek(512)
                with open("kernel.bin", "rb") as k:
                    f.write(k.read())
            self.log("镜像制作完成", "green")
        except Exception as e:
            self.log(f"写入失败: {e}", "red")

    def make_iso(self):
        cwd = self.entry_dir.get() or os.getcwd()
        if not os.path.exists("kernel.elf"):
            self.log("kernel.elf 不存在，请先链接内核", "red")
            return

        # 创建 ISO 目录结构
        iso_dir = os.path.join(cwd, "iso")
        if os.path.exists(iso_dir):
            shutil.rmtree(iso_dir)
        os.makedirs(os.path.join(iso_dir, "boot", "grub"))

        shutil.copy("kernel.elf", os.path.join(iso_dir, "boot", "kernel.elf"))

        with open(os.path.join(iso_dir, "boot", "grub", "grub.cfg"), "w") as f:
            f.write("""
set timeout=5
set default=0
menuentry "Mpple Kernel" {
    multiboot /boot/kernel.elf
    boot
}
""")

        cmd = ["grub-mkrescue", "-o", "MppleKernel.iso", iso_dir]
        r, _ = self.run_cmd(cmd, cwd=cwd)
        shutil.rmtree(iso_dir, ignore_errors=True)
        if r:
            self.log("ISO 制作完成: MppleKernel.iso", "green")
        else:
            self.log("ISO 制作失败", "red")

    def run_qemu(self):
        qemu = self.entry_qemu.get().strip() or "qemu-system-i386"
        cwd = self.entry_dir.get() or os.getcwd()
        iso = os.path.join(cwd, "MppleKernel.iso")
        floppy = os.path.join(cwd, "MppleKernel.img")
        if os.path.exists(iso):
            self.run_cmd([qemu, "-cdrom", iso, "-snapshot"], capture=False)
        elif os.path.exists(floppy):
            self.run_cmd([qemu, "-drive", f"format=raw,file={floppy}", "-fda", floppy, "-snapshot"], capture=False)
        else:
            self.log("未找到镜像文件", "red")

    def stop_qemu(self):
        if self.qemu_process and self.qemu_process.poll() is None:
            self.qemu_process.terminate()
            self.log("已终止 QEMU", "orange")

    def run_all(self):
        self.compile_kernel()
        self.link_kernel()
        self.objcopy_kernel()

    def clean_files(self):
        cwd = self.entry_dir.get() or os.getcwd()
        patterns = ['*.o', '*.elf', '*.bin', 'MppleKernel.img', 'MppleKernel.iso']
        for p in patterns:
            for f in glob.glob(os.path.join(cwd, p)):
                try:
                    os.remove(f)
                    self.log(f"删除 {os.path.basename(f)}", "green")
                except Exception as e:
                    self.log(f"删除失败 {f}: {e}", "red")
            if p == 'MppleKernel.iso':
                iso_dir = os.path.join(cwd, 'iso')
                if os.path.isdir(iso_dir):
                    try:
                        shutil.rmtree(iso_dir)
                        self.log("删除 iso 目录", "green")
                    except Exception as e:
                        self.log(f"删除 iso 目录失败: {e}", "red")

if __name__ == "__main__":
    app = BuildUI()
    app.mainloop()