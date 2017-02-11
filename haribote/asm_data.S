.section .data
		
.align 4
.globl _FrameBufferInfo 
_FrameBufferInfo:
.int 800 /* #0 Physical Width */
.int 480 /* #4 Physical Height */
.int 800 /* #8 Virtual Width */
.int 480 /* #12 Virtual Height */
.int 0 /* #16 GPU - Pitch */
.int 16 /* #20 Bit Depth */
.int 0 /* #24 X */
.int 0 /* #28 Y */
.int 0 /* #32 GPU - Pointer */
.int 0 /* #36 GPU - Size */						

.globl _cons_cons // store the pointer to the sheet that you want to put a letter
_cons_cons:		.word	0
.globl _cons_shtctl
_cons_shtctl:	.word	0
		
.align 4
.globl hankaku
hankaku:
	.incbin "hankaku.bin"
