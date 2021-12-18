init:
	add  $r3,   $zero, $zero, 0
	add  $r2,   $zero, $imm,  0x200

loop:
	lw   $r4,   $r3,   $zero, 0  # r[4]=mem[r[3]]
	add  $r3,   $r4,   $imm,  1  # r[3]=r[4]+1
	sw   $r3,   $r4,   $zero, 0  # mem[r[4]]=r[3]	
	ble  $imm,  $r3,   $r2,   loop

exit:
	halt $zero, $zero, $zero, 0
	halt $zero, $zero, $zero, 0
	halt $zero, $zero, $zero, 0
	halt $zero, $zero, $zero, 0
	halt $zero, $zero, $zero, 0
