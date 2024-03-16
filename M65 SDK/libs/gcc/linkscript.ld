__ROM_START_ADDR = 0x88310000;
__MAX_ROM_SIZE   = 320*1024;

__RAM_START_ADDR = 0x823E7000;
__RAM_END_ADDR   = 0x82400000;

SECTIONS
{
	. = __ROM_START_ADDR;
	
	_ROM_START =.;
	.initdata . : AT(ADDR(.initdata))
	{
		*(.initdata)
		. = ALIGN(0x10);
	}

	.text . : AT(ADDR(.text))
	{ 
	    *(.text) 
		. = ALIGN(0x10);	    
	} 
	.rodata . : AT(ADDR(.rodata))
	{
	    *(.rodata)
	    *(.rodata.*)	
	    *(.rel.dyn) 
		. = ALIGN(0x10);	       
	}	

	_ROM_RW_DATA_START = .;
	. = __RAM_START_ADDR;
	
	.app_RW_data _RAM_RW_DATA_START : AT(_ROM_RW_DATA_START)
	{
		_RAM_RW_DATA_START = .;
		*(.data)
		*(.data.*)
		*(.sdata)
		. = ALIGN(0x10);
	}
	
	_RAM_RW_DATA_END = .;

	_ROM_RW_DATA_SIZE = _RAM_RW_DATA_END - _RAM_RW_DATA_START;

	_RAM_BSS_START = _ROM_RW_DATA_START + _ROM_RW_DATA_SIZE;

	_ROM_END  = _RAM_BSS_START;

	_ROM_SIZE = _ROM_END - _ROM_START;


	ASSERT(_ROM_SIZE < __MAX_ROM_SIZE,"APP ROM OVERFLOW !!")
	
	/*-------------------------------------------------------  */
    /* Uninitialized section in RAM (content kept after reboot)*/
    /*-------------------------------------------------------  */
    .backup . (NOLOAD) : AT (ADDR(.backup))
    {
        *(.backup)
        . = ALIGN(0x4);
    }

	.app_ZI_data . : AT(_RAM_BSS_START)
	{
		_RAM_BSS_START = .;
		*(.bss)
		*(.sbss)
		*(.bss.*)			
		*(COMMON)
		*(.scommon)
		. = ALIGN(0x10);
	}
	
	_RAM_BSS_END = .;

	_RAM_BSS_SIZE = _RAM_BSS_END - _RAM_BSS_START;

	ASSERT(_ROM_RW_DATA_SIZE +  _RAM_BSS_SIZE < __RAM_END_ADDR - __RAM_START_ADDR,"APP RAM OVERFLOW !!")
}
