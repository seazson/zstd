#define CFG_INIT_RAM_ADDR       0x00070000
#define CFG_INIT_SP_OFFSET      0x1000

.globl soc_init
soc_init:
	ldr	sp, TMP_STACK	/* set up a temporary stack */

	stmdb	sp!, {ip, lr}
	bl	bsp_init	/* go setup pll, memory */
	ldmia	sp!, {ip, pc}	/* restore registers and return to caller */

/* the literal pools origin */
	.ltorg
TMP_STACK:
.word	CFG_INIT_RAM_ADDR + CFG_INIT_SP_OFFSET

