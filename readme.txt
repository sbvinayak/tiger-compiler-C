/* 
 * Assignment 6
 * 
 * Readme file for TIGER's 
 *   Assembly Code Generation Module
 *   CFG Generation Module
 *
 * Ming Zhou (4465225)
 * Kuo-shih Tseng (4436736)
 */

[Release notes]

1, both instructions and pseudo-instructions are used in code generation.
2, among the registers that the callee must trash, for now only a part of them
are saved into the generic assembly code structure (in dst field). This needs 
to be fixed later, after the register allocation is done.
3, minor changes are made to Util, Main and other modules to enhance the 
modularity and integrity. To output CFG in a more human-readable way, changes 
are also made to Graph module.

[MIPS instruction references]

MIPS instructions			Assembly code		Meaning
Add 					add $d,$s,$t 		$d = $s + $t
Subtract 				sub $d,$s,$t 		$d = $s - $t
Add immediate 				addi $t,$s,C 		$t = $s + C (signed)
Load word 				ld $t,C($s) 		$t = Memory[$s + C]
Store word 				sw $t,C($s) 		Memory[$s + C] = $t
And 					and $d,$s,$t 		$d = $s & $t
And immediate 				andi $t,$s,C 		$t = $s & C
Or 					or $d,$s,$t 		$d = $s | $t
Or immediate 				ori $t,$s,C 		$t = $s | C
Shift left logical 			sll $d,$t,shamt 	$d = $t << shamt
Shift right logical 			srl $d,$t,shamt 	$d = $t >> shamt
Shift right arithmetic 			sra $d,$t,shamt
Branch on equal 			beq $s,$t,C 		if ($s == $t) go to PC+4+4*C
Branch on not equal 			bne $s,$t,C 		if ($s != $t) go to PC+4+4*C
Jump 					j C 			PC = PC+4[31:28] . C*4
Jump register 				jr $s 			goto address $s
Jump and link 				jal C 			$31 = PC + 8; PC = PC+4[31:28] . C*4
	(For procedure call - used to call a subroutine, $31 holds the return address; returning from 
	a subroutine is done by: jr $31. Return address is PC + 8, not PC + 4 due to the use of a 
	branch delay slot which forces the instruction after the jump to be executed)
Move 					move $rt,$rs 		addi $rt,$rs,0
Branch if greater than 			bgt $rs,$rt,Label 	
Branch if less than 			blt $rs,$rt,Label 	
Branch if greater than or equal 	bge $rs,$rt,Label 	
Branch if less than or equal 		ble $rs,$rt,Label
Multiplies and returns only first 32 bits 	mul $d, $s, $t 	
Divides and returns quotient 		div $d, $s, $t 	

