MEMORY
{
  FLASH (rx)      : ORIGIN = 0x08008000, LENGTH = 106K
  TCMSRAM (rw)    : ORIGIN = 0x10000000, LENGTH = 64K
  RAM (xrw)       : ORIGIN = 0x20000000, LENGTH = 112K
  RAM1 (xrw)      : ORIGIN = 0x2001C000, LENGTH = 16K
  RAM2 (xrw)      : ORIGIN = 0x20020000, LENGTH = 0
  RAMADD (xrw)    : ORIGIN = 0x20030000, LENGTH = 0
  BKPSRAM (rw)    : ORIGIN = 0x40024000, LENGTH = 4K
}

ENTRY(Reset_Handler)

SECTIONS
{
  __heap_size = DEFINED(__heap_size) ? __heap_size : 4K;
  __stack_size = DEFINED(__stack_size) ? __stack_size : 10K;

  .vectors :
  {
    . = ALIGN(4);
    KEEP(*(.vectors))
    . = ALIGN(4);
    __Vectors_End = .;
    __Vectors_Size = __Vectors_End - __gVectors;
  } >FLASH

  .text :
  {
    . = ALIGN(4);
	*(.text.unlikely*)
	*(.text.hot*)
	*(.text)
	*(.text*)
	*(.glue_7)
	*(.glue_7t)
	*(.eh_frame)
    KEEP (*(.init))
    KEEP (*(.fini))
    . = ALIGN(4);
    _etext = .;
    
    . = ALIGN(8);
     
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP (*(.preinit_array*))
    PROVIDE_HIDDEN (__preinit_array_end = .);

    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(SORT(.init_array.*)))
    KEEP (*(.init_array*))
    PROVIDE_HIDDEN (__init_array_end = .);

    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP (*(.fini_array*))
    KEEP (*(SORT(.fini_array.*)))
    PROVIDE_HIDDEN (__fini_array_end = .);
    
  } >FLASH

  .rodata :
  {
    . = ALIGN(4);
    *(.rodata)
    *(.rodata*)
    . = ALIGN(4);
  } >FLASH

  .stack :
  {
	. = ALIGN(4);
	PROVIDE( stack_low = . );
    . = . + __stack_size;  
    PROVIDE( _sp = . ); 
    . = ALIGN(4);
  } >TCMSRAM

  .tcmsram :
  {
  	. = ALIGN(4);
    *(.tcmsram)
    *(.tcmsram*)
    . = ALIGN(4);
    _snetwork = .;	
	*(.network*)
    . = ALIGN(4);
	_enetwork = .;	
    . = ALIGN(4);
  } >TCMSRAM
  
  .heap :
  {
    . = ALIGN(4);
    heap_low = .;
    . = . + __heap_size;
    heap_top = .;
    . = ALIGN(4);
  } >TCMSRAM

  _sidata = LOADADDR(.data);
  .data :
  {
    . = ALIGN(4);
    _sdata = .;
    *(.data)
    *(.data*)
    . = ALIGN(4);
    _edata = .;
  } >RAM AT> FLASH

  . = ALIGN(4);
  .bss :
  {
    _sbss = .;
    __bss_start__ = _sbss;
    *(.bss)
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    _ebss = .;
    __bss_end__ = _ebss;
  } >RAM

  . = ALIGN(8);
  PROVIDE ( end = _ebss );
  PROVIDE ( _end = _ebss );
  
  .sram1 :
  {
  	. = ALIGN(4);
    *(.sram1)
    *(.sram1*)
	. = ALIGN(4);
    _sdmx = .;
    *(.dmx)
	. = ALIGN(4);
    _edmx = .;
	. = ALIGN(4);
	_spixel = .;
    *(.pixel)
	. = ALIGN(4);
	_epixel = .;
  } >RAM1 
  
  .sram2 :
  {
  	. = ALIGN(4);
    *(.sram2)
    *(.sram2*)
  } >RAM2 

  .ramadd :
  {
    _sramadd = .;
  	. = ALIGN(4);
    *(.ramadd)
    *(.ramadd*)
	. = ALIGN(4);
    _eramadd = .;
  } >RAMADD AT>RAMADD

  .bkpsram :
  {
  	. = ALIGN(4);
    *(.bkpsram)
    *(.bkpsram*)
    *(.configstore*)
  } >BKPSRAM AT>BKPSRAM

  /DISCARD/ :
  {
	*(*.ARM.*)
	*(*.comment)
	*(*.debug*)
  }
}

 /* input sections */
GROUP(libgcc.a libc.a)