/* BENCH FOUNDATION ONLY; NOT the immutable-loader/slot OTA linker layout.
 * STM32G474CEU6: 512 KiB Flash, 96 KiB SRAM, 32 KiB CCM SRAM. */
ENTRY(Reset_Handler)

_Min_Heap_Size = 0;
_Min_Stack_Size = 0x2000;

MEMORY
{
  FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 512K
  RAM   (rw)  : ORIGIN = 0x20000000, LENGTH = 96K
  CCMRAM (rw) : ORIGIN = 0x10000000, LENGTH = 32K
}

_estack = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS
{
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector))
    . = ALIGN(4);
  } > FLASH

  .note.gnu.build-id :
  {
    /* GNU ELF note: 12-byte header + 4-byte owner + 20-byte SHA-1. */
    KEEP(*(.note.gnu.build-id))
  } > FLASH
  canview_stm_link_build_id = ADDR(.note.gnu.build-id) + 16;

  .text :
  {
    . = ALIGN(4);
    *(.text)
    *(.text*)
    *(.glue_7)
    *(.glue_7t)
    *(.eh_frame)
    KEEP(*(.init))
    KEEP(*(.fini))
    . = ALIGN(4);
    _etext = .;
  } > FLASH

  .rodata :
  {
    . = ALIGN(4);
    *(.rodata)
    *(.rodata*)
    . = ALIGN(4);
  } > FLASH

  .ARM.extab :
  {
    *(.ARM.extab* .gnu.linkonce.armextab.*)
  } > FLASH

  .ARM.exidx :
  {
    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;
  } > FLASH

  .preinit_array (READONLY) :
  {
    PROVIDE_HIDDEN(__preinit_array_start = .);
    KEEP(*(.preinit_array*))
    PROVIDE_HIDDEN(__preinit_array_end = .);
  } > FLASH

  .init_array (READONLY) :
  {
    PROVIDE_HIDDEN(__init_array_start = .);
    KEEP(*(SORT(.init_array.*)))
    KEEP(*(.init_array*))
    PROVIDE_HIDDEN(__init_array_end = .);
  } > FLASH

  .fini_array (READONLY) :
  {
    PROVIDE_HIDDEN(__fini_array_start = .);
    KEEP(*(SORT(.fini_array.*)))
    KEEP(*(.fini_array*))
    PROVIDE_HIDDEN(__fini_array_end = .);
  } > FLASH

  _sidata = LOADADDR(.data);

  .data :
  {
    . = ALIGN(4);
    _sdata = .;
    *(.data)
    *(.data*)
    . = ALIGN(4);
    _edata = .;
  } > RAM AT > FLASH

  .ccmram (NOLOAD) :
  {
    . = ALIGN(4);
    _sccmram = .;
    *(.ccmram)
    *(.ccmram*)
    . = ALIGN(4);
    _eccmram = .;
  } > CCMRAM

  .bss (NOLOAD) :
  {
    . = ALIGN(4);
    _sbss = .;
    __bss_start__ = _sbss;
    *(.bss)
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    _ebss = .;
    __bss_end__ = _ebss;
  } > RAM

  ._user_heap_stack (NOLOAD) :
  {
    . = ALIGN(8);
    PROVIDE(end = .);
    PROVIDE(_end = .);
    . = . + _Min_Heap_Size;
    . = . + _Min_Stack_Size;
    . = ALIGN(8);
  } > RAM

/* Cortex-M stack grows downward from the high end of RAM. The watermark must
 * cover the reserved high-end stack window, not the low-end section address. */
__stack_limit = _estack - _Min_Stack_Size;

  .ARM.attributes 0 : { *(.ARM.attributes) }
}

ASSERT(SIZEOF(.data) + SIZEOF(.bss) + SIZEOF(.ccmram) <= 80K, "STM32 static RAM budget exceeded")
ASSERT(SIZEOF(.note.gnu.build-id) == 36, "STM32 SHA-1 build ID note missing or invalid")
ASSERT(_Min_Stack_Size <= 24K, "STM32 reserved stack budget exceeded")
ASSERT(128K - SIZEOF(.data) - SIZEOF(.bss) - SIZEOF(.ccmram) - _Min_Stack_Size >= 24K, "STM32 RAM margin below 24 KiB")
