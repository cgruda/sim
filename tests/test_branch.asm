test_beq:
	add $r2, $zero, $imm, 3
	add $r3, $zero, $imm, 3
	beq $imm, $r2, $r3, test_bne
	add $zero, $zero, $zero, 0

test_blt:
	add $r2, $zero, $imm, -20
	add $r3, $zero, $imm, 0
	blt $imm, $r2, $r3, test_bgt
	add $zero, $zero, $zero, 0

test_bge:
	add $r2, $zero, $imm, -7
	add $r3, $zero, $imm, 3
	bge $imm, $r3, $r2, exit
	add $zero, $zero, $zero, 0

test_bne:
	add $r2, $zero, $imm, 2
	add $r3, $zero, $imm, 3
	bne $imm, $r2, $r3, test_blt
	add $zero, $zero, $zero, 0

test_ble:
	add $r2, $zero, $imm, -8
	add $r3, $zero, $imm, -1
	ble $imm, $r2, $r3, test_bge
	add $zero, $zero, $zero, 0

test_bgt:
	add $r2, $zero, $imm, 2
	add $r3, $zero, $imm, 3
	bgt $imm, $r3, $r2, test_ble
	add $zero, $zero, $zero, 0
	
exit:
	halt $zero, $zero, $zero, 0	# PC=8
	halt $zero, $zero, $zero, 0	# PC=9
	halt $zero, $zero, $zero, 0	# PC=10
	halt $zero, $zero, $zero, 0	# PC=11
	halt $zero, $zero, $zero, 0	# PC=12
