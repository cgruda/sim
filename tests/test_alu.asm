	add $r2, $zero, $imm, 100	# PC=0
	sub $r3, $r2, $imm, 150
	add $r4, $zero, $imm, 0xFFF
	and $r4, $r4, $imm, 0xF0F
	or  $r4, $r4, $imm, 0xF6F
	xor $r4, $r4, $imm, 0x891
	mul $r5, $r3, $imm, -2
	add $r6, $zero, $imm, 1
	sll $r6, $r6, $imm, 3
	sra $r6, $r3, $imm, 4
	srl $r6, $r3, $imm, 4
	halt $zero, $zero, $zero, 0	# PC=8
	halt $zero, $zero, $zero, 0	# PC=9
	halt $zero, $zero, $zero, 0	# PC=10
	halt $zero, $zero, $zero, 0	# PC=11
	halt $zero, $zero, $zero, 0	# PC=12
