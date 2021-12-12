	blt	$zero,	$r8,	$imm,	12		# if(r[8]<10) goto 0
	add	$r8,	$r8,	$imm,	1		# r[8]++ (delay slot)
	add $r6,	$zero,	$imm,	127		# r[6]=127 (incmt count)
incmt:
	add $r7,	$zero,	$imm,	20		# r[7]=25 ("sleep")
	lw	$r2,	$zero,	$zero,	0		# r[2]=mem[0]
	add	$r2,	$r2,	$imm,	1		# r[2]++
	sw	$r2,	$zero,	$zero,	0		# mem[0]=r[2]
sleep:
	bgt	$imm,	$r7,	$zero,	sleep	# if(r[7]>0) goto sleep
	sub	$r7,	$r7,	$imm,	1		# r[3]++ (delay slot)
	blt	$imm,	$r3,	$r6,	incmt	# if(r[3]<127) goto incmt
	add	$r3,	$r3,	$imm,	1		# r[3]++ (delay slot)
	halt $zero, $zero,	$zero,	0
	halt $zero, $zero,	$zero,	0
	halt $zero, $zero,	$zero,	0
	halt $zero, $zero,	$zero,	0
	halt $zero, $zero,	$zero,	0
