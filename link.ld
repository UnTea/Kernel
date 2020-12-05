/* link.ld */
OUTPUT_FORMAT(elf32-i386)
ENTRY(start)
SECTIONS
{
    . = 0x100000;           /*kernel code should start from here*/
    .text : { *(.text) }
    .data : { *(.data) }
    .bss  : { *(.bss)  }
}

/*ld -m elf_i386 -T link.ld -o kernel kasm.o kc.o*/