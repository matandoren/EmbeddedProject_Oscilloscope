
#include <p32xxxx.h>

	/* define all global symbols here */
.global main
.global input

	/* define all global symbols here */
.data
input:	.byte 0
      

.text
     /* This is important for an assembly programmer. This
     * directive tells the assembler that don't optimize
     * the order of the instructions as well as don't insert
     * 'nop' instructions after jumps and branches.
    */
	.set noreorder
/************************************************************************
* main()
 * This is where the PIC32 start-up code will jump to after initial
 * set-up.
 ********************************************************************/
/*.ent main /* directive that marks symbol 'main' as function in ELF
           * output
           */
main: 
	/*TRISF&=0xfeff;*/
	lw		$t0, TRISF
	andi	$t0,$t0,0xfeff
	sw		$t0, TRISF 
	/******************/
	/*tmp=PORTF & 0xfeff;*/
	lw		$t0, PORTF
	andi	$t0,$t0,0xfeff
	sw		$t0,PORTF
	/***********************/
    jal		readSwitches
	nop
	jal		lightLEDS
	nop
	j		main
	nop
/*****************************/

.ent readSwitches
readSwitches:  
	/*TRISF&=0xfff8;*/
	lw		$t0, TRISF
	andi	$t0,$t0,0xfff8
	sw		$t0, TRISF
	/*TRISE|=0x00ff;*/
	lw		$t0, TRISE
	ori		$t0,$t0,0x00ff
	sw		$t0, TRISE
	/*TRISD&=0xffcf;*/
	lw		$t0, TRISD
	andi	$t0,$t0,0xffcf
	sw		$t0, TRISD
	/*tmp=PORTF & 0xfff8;
    /*PORTF=tmp | 0x3;*/
	lw		$t0, PORTF
	andi	$t0,$t0,0xfff8
	ori		$t0,$t0,0x3
	sw		$t0,PORTF
	/*PORTDbits.RD4=1;*/
	lw		$t0, PORTD
	ori		$t0,$t0,0x0010
	sw		$t0, PORTD
	/*PORTDbits.RD4=0;*/
	lw		$t0, PORTD
	andi	$t0,$t0,0xffef
	sw		$t0, PORTD
	/***********************/
	lw		$t0, PORTE
	sb		$t0,input
	jr		$ra
	nop
/**********************************/
.end readSwitches

.ent lightLEDS
lightLEDS:
	/*TRISF&=0xfff8;*/
	lw		$t0, TRISF
	andi	$t0,$t0,0xfff8
	sw		$t0, TRISF
	/*TRISE&=0xff00;*/
	lw		$t0, TRISE
	andi	$t0,$t0,0xff00
	sw		$t0, TRISE
	/*TRISD&=0xffcf;*/
	lw		$t0, TRISD
	andi	$t0,$t0,0xffcf
	sw		$t0, TRISD
    /*tmp=PORTF & 0xfff8;*/
	/*PORTF=tmp | 0x4;*/
	lw		$t0, PORTF
	andi	$t0,$t0,0xfff8
	ori		$t0,$t0,0x4
	sw		$t0,PORTF
	/*tmp=PORTE &0xff00;
	PORTE=tmp | LEDS;*/
	lw		$t0,PORTE
	andi	$t0,$t0,0xff00
	lbu		$t1,input
	or		$t0,$t0,$t1
	sw		$t0,PORTE
	/*PORTDbits.RD4=1;*/
	lw		$t0, PORTD
	ori		$t0,$t0,0x0010
	sw		$t0, PORTD
	/*PORTDbits.RD4=0;*/
	lw		$t0, PORTD
	andi	$t0,$t0,0xffef
	sw		$t0, PORTD
	/***********************/
	jr		$ra
	nop
/**********************************/
.end lightLEDS
