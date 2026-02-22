OUTPUT_FORMAT(elf32-i386)
ENTRY(kernel_main)

SECTIONS
{
    . = 0x10000;  /* 内核加载地址 */

    .text : {
        *(.text)
    }

    .rodata : {
        *(.rodata)
    }

    .data : {
        *(.data)
    }

    .bss : {
        *(.bss)
    }
}