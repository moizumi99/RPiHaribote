ENTRY(HariMain)

MEMORY {
	   ram.y(rxw) : ORIGIN = 0x80008000, LENGTH = 0x00100000
}
 
SECTIONS
{
    .text :
    {
    }
    .data :
    {
    }
    __bss_start__ = .;
    .bss :
    {
    }
    __bss_end__ = .;
	.others :
	{
	}
    __end = .;
}
