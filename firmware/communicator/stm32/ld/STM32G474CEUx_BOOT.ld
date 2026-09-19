/* MCUboot 고정 64KiB. SDK Reset_Handler가 SRAM 코드와 .data를 함께 복사한다. */
ENTRY(Reset_Handler)
MEMORY
{
  FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 64K
  RAM (rw) : ORIGIN = 0x20000000, LENGTH = 96K
}
/* 실행 코드와 mutable data를 같은 RWX ELF segment로 합치지 않는다. */
PHDRS
{
  flash PT_LOAD FLAGS(5);
  ram_code PT_LOAD FLAGS(5);
  ram_data PT_LOAD FLAGS(6);
}
_Min_Heap_Size = 0;
_Min_Stack_Size = 0x2000;
_estack = ORIGIN(RAM) + LENGTH(RAM);
__stack_limit = _estack - _Min_Stack_Size;
SECTIONS
{
  .isr_vector : { KEEP(*(.isr_vector)) } > FLASH :flash
  .note.gnu.build-id : { KEEP(*(.note.gnu.build-id)) } > FLASH :flash
  .text :
  {
    *(.text .text.*) *(.glue_7 .glue_7t)
    KEEP(*(.init)) KEEP(*(.fini))
    . = ALIGN(4);
  } > FLASH :flash
  .rodata : { *(.rodata .rodata.*) . = ALIGN(4); } > FLASH :flash
  .ARM.extab : { *(.ARM.extab* .gnu.linkonce.armextab.*) } > FLASH :flash
  .ARM.exidx :
  {
    __exidx_start = .; *(.ARM.exidx*) __exidx_end = .;
  } > FLASH :flash
  .preinit_array (READONLY) :
  {
    PROVIDE_HIDDEN(__preinit_array_start = .);
    KEEP(*(.preinit_array*))
    PROVIDE_HIDDEN(__preinit_array_end = .);
  } > FLASH :flash
  .init_array (READONLY) :
  {
    PROVIDE_HIDDEN(__init_array_start = .);
    KEEP(*(SORT(.init_array.*))) KEEP(*(.init_array*))
    PROVIDE_HIDDEN(__init_array_end = .);
  } > FLASH :flash
  .fini_array (READONLY) :
  {
    PROVIDE_HIDDEN(__fini_array_start = .);
    KEEP(*(SORT(.fini_array.*))) KEEP(*(.fini_array*))
    PROVIDE_HIDDEN(__fini_array_end = .);
  } > FLASH :flash
  /* SDK SystemInit보다 앞선 errata wrapper만 쓰며 load/copy 대상은 아니다. */
  .startup_sram_dummy (NOLOAD) :
  {
    KEEP(*(.startup_sram_dummy))
  } > RAM :NONE
  .canview_flash_ram :
  {
    _sdata = .;
    KEEP(*(.canview_flash_ram))
    . = ALIGN(4);
  } > RAM AT > FLASH :ram_code
  .canview_flash_read_ram :
  {
    KEEP(*(.canview_flash_read_ram))
    . = ALIGN(4);
  } > RAM AT > FLASH :ram_code
  .data :
  {
    *(.data .data.*)
    . = ALIGN(4);
    _edata = .;
  } > RAM AT > FLASH :ram_data
  _sidata = LOADADDR(.canview_flash_ram);
  .bss (NOLOAD) :
  {
    _sbss = .; __bss_start__ = .;
    *(.bss .bss.*) *(COMMON)
    . = ALIGN(4);
    _ebss = .; __bss_end__ = .;
  } > RAM :ram_data
  .ccmram (NOLOAD) : { *(.ccmram .ccmram.*) } > RAM :NONE
  _sccmram = 0x10000000;
  _eccmram = _sccmram;
  ._user_heap_stack (NOLOAD) :
  {
    . = ALIGN(8);
    PROVIDE(end = .); PROVIDE(_end = .);
    . += _Min_Stack_Size;
  } > RAM :NONE
  .ARM.attributes 0 : { *(.ARM.attributes) }
}
ASSERT(ADDR(.isr_vector) == 0x08000000 && SIZEOF(.isr_vector) == 0x1d8, "boot vector invalid")
ASSERT(SIZEOF(.note.gnu.build-id) == 36, "boot SHA-1 build ID required")
ASSERT(ADDR(.startup_sram_dummy) == 0x20000000 && SIZEOF(.startup_sram_dummy) == 4, "boot startup dummy invalid")
ASSERT(SIZEOF(.canview_flash_ram) > 0 && SIZEOF(.canview_flash_ram) <= 2048, "boot command SRAM missing/oversize")
ASSERT(SIZEOF(.canview_flash_read_ram) > 0 && SIZEOF(.canview_flash_read_ram) <= 2048, "boot read SRAM missing/oversize")
ASSERT(ADDR(.canview_flash_read_ram) == ADDR(.canview_flash_ram) + SIZEOF(.canview_flash_ram), "boot SRAM code VMA gap")
ASSERT(ADDR(.data) == ADDR(.canview_flash_read_ram) + SIZEOF(.canview_flash_read_ram), "boot data VMA gap")
ASSERT(LOADADDR(.canview_flash_read_ram) == _sidata + SIZEOF(.canview_flash_ram), "boot SRAM code LMA gap")
ASSERT(LOADADDR(.data) == LOADADDR(.canview_flash_read_ram) + SIZEOF(.canview_flash_read_ram), "boot data LMA gap")
ASSERT((_sdata & 3) == 0 && (_edata & 3) == 0 && (_sidata & 3) == 0, "boot SDK copy alignment")
ASSERT(SIZEOF(.ccmram) == 0, "boot CCM parity initialization not implemented")
ASSERT(_ebss - ORIGIN(RAM) <= 80K, "boot static RAM budget exceeded")
ASSERT(_ebss <= __stack_limit, "boot static data overlaps stack")
ASSERT(128K - (_ebss - ORIGIN(RAM)) - _Min_Stack_Size >= 24K, "boot RAM margin below 24KiB")
