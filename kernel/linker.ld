OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)

ENTRY(keMain)

PHDRS
{
    text    PT_LOAD    FLAGS((1 << 0) | (1 << 2)) ; /* Execute + Read */
    rodata  PT_LOAD    FLAGS((1 << 2)) ;            /* Read only */
    data    PT_LOAD    FLAGS((1 << 1) | (1 << 2)) ; /* Write + Read */
}

SECTIONS
{
    . = 0xffffffff80000000;

    .text : {
        *(.text .text.*)
    } :text

    . += CONSTANT(MAXPAGESIZE);

    .rodata : {
        *(SORT(.init_array*));  /* Note the "SORT" */

        *(SORT(.fini_array*));

        *(.rodata .rodata.*)
        *(.gnu.linkonce.r*)
    } :rodata

    . += CONSTANT(MAXPAGESIZE);

    .data : {
        *(.data .data.*)
        *(.gnu.linkonce.d*)
    } :data

    .bss : {
        *(COMMON)
        *(.bss .bss.*)
        *(.gnu.linkonce.b*)
    } :data

    /DISCARD/ : {
        *(.eh_frame)
        *(.note .note.*)
    }
}
