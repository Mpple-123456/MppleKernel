g++ -m32 -ffreestanding -nostdlib -fno-builtin -fno-exceptions -fno-rtti -O2 -c kernel.cpp -o kernel.o -std=c++11
ld -m elf_i386 -T linker.ld -o kernel.elf kernel.o
objcopy -O binary kernel.elf kernel.bin

# 合并内核与所有程序（按地址顺序）
nasm -f bin boot.asm -o boot.bin
dd if=/dev/zero of=os.img bs=512 count=2880
dd if=boot.bin of=os.img conv=notrunc
dd if=kernel.bin of=os.img seek=1 conv=notrunc
qemu-system-i386 -drive format=raw,file=os.img -fda os.img -snapshot