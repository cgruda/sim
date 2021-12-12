	blt	$zero,	$r8,	$imm,	5	# if(r[8]<5) goto 0 (delay core1)
	add	$r8,	$r8,	$imm,	1	# r[8]++ (delay slot)
	add	$r6,	$zero,	$imm,	127	# r[6]=127 (incmt count)
	add	$zero,	$zero,	$zero,	0	# sleep once clock (due to core0)
incmt:
	add	$r7,	$zero,	$imm,	20	# r[7]=20 (for sleep)
	lw	$r2,	$zero,	$zero,	0	# r[2]=mem[0]
	add	$r2,	$r2,	$imm,	1	# r[2]++
	sw	$r2,	$zero,	$zero,	0	# mem[0]=r[2]
sleep:
	bgt	$imm,	$r7,	$zero,	sleep	# if(r[7]>0) goto sleep
	sub	$r7,	$r7,	$imm,	1	# r[7]++ (delay slot)
	blt	$imm,	$r3,	$r6,	incmt	# if(r[3]<r[6]) goto incmt
	add	$r3,	$r3,	$imm,	1	# r[3]++ (delay slot)
	halt	$zero,	$zero,	$zero,	0
	halt	$zero,	$zero,	$zero,	0
	halt	$zero,	$zero,	$zero,	0
	halt	$zero,	$zero,	$zero,	0
	halt	$zero,	$zero,	$zero,	0
