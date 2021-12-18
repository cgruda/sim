	add  $r2,	$zero, $imm, 0x2
	add  $r3,	$zero, $imm, 0x3
	add  $r4,	$zero, $imm, 0x4
	add  $r5,	$zero, $imm, 0x5
	add  $r6,	$zero, $imm, 0x6
	add  $r7,	$zero, $imm, 0x7
	add  $r8,	$zero, $imm, 0x8
	add  $r9,	$zero, $imm, 0x9
	add  $r10,	$zero, $imm, 0xA
	add  $r11,	$zero, $imm, 0xB
	sw   $imm,  $zero, $r2,  0x222 # mem[2]=0x222
	sw   $imm,  $zero, $r3,  0x333 # mem[3]=0x333
	sw   $imm,  $zero, $r4,  0x444 # mem[4]=0x444
	lw   $r12,  $zero, $imm, 0xB   # r[12]=mem[0xB]
	sw   $r11,	$zero, $imm, 0xAAA # mem[0xB]=r[11]
	halt $zero, $zero, $zero, 0
	halt $zero, $zero, $zero, 0
	halt $zero, $zero, $zero, 0
	halt $zero, $zero, $zero, 0
	halt $zero, $zero, $zero, 0
