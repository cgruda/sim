	lw   $r2,   $zero, $imm,  0      # r[2]=mem[0]
	lw   $r3,   $zero, $imm,  1 	 # r[3]=mem[1]
	lw   $r4,   $zero, $imm,  2  	 # r[4]=mem[2]
	lw   $r5,   $zero, $imm,  3  	 # r[5]=mem[3]
	lw   $r6,   $zero, $imm,  4 	 # r[6]=mem[4]
	lw   $r7,   $zero, $imm,  0x100  # r[6]=mem[0x100]
	lw   $r8,   $zero, $imm,  0x200  # r[6]=mem[0x200]
	halt $zero, $zero, $zero, 0
	halt $zero, $zero, $zero, 0
	halt $zero, $zero, $zero, 0
	halt $zero, $zero, $zero, 0
	halt $zero, $zero, $zero, 0
