OUTPUT_FORMAT("elf64-powerpcle", "elf64-powerpcle", "elf64-powerpcle")
OUTPUT_ARCH(powerpc:common64)

/* set the entry point */
ENTRY ( _start )

SECTIONS {

  .text :  {
             *(.text)
  }


}
