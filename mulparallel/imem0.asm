	add	$fp,	$zero,	$imm,	1	# pc0
	add	$sp,	$zero,	$imm,	16	# pc1
	add	$s0,	$zero,	$imm,	0x00000	# pc2	A base addr
	add	$s1,	$zero,	$imm,	0x00100	# pc3	B base addr
	add	$s2,	$zero,	$imm,	0x00200	# pc4	C base addr

	add	$a0,	$zero,	$zero,	0	# pc5	reset n=0
nLoop:	add	$a1,	$zero,	$zero,	0	# pc6	reset m=0
mLoop:	beq	$imm,	$zero,	$zero,	Calc	# pc7	Calc C(n,m)
	add	$zero,	$zero,	$zero,	0	# pc8	(delay slot)
Next:	add	$a1,	$a1,	$fp,	0	# pc9	m++
	add	$t0,	$zero,	$imm,	4	# pcA
	blt	$imm,	$a1,	$t0,	mLoop	# pcB	if (m<4) go to mLoop
	add	$zero,	$zero,	$zero,	0	# pcC	(delay slot)
	add	$a0,	$a0,	$fp,	0	# pcD	n++
	add	$t0,	$zero,	$sp,	0	# pcE
	blt	$imm,	$a0,	$t0,	nLoop	# pcF	if (0<=n<16) go to nLoop
	add	$zero,	$zero,	$zero,	0	# pc10	(delay slot)
	lw	$t1,	$imm,	$zero,	0x3F	# pc11	ecivt block idx 15
	halt	$zero,	$zero,	$zero,	0	# pc12	exit program
	halt	$zero,	$zero,	$zero,	0	# pc13	n=4, m=4, exit program
	halt	$zero,	$zero,	$zero,	0	# pc14	n=4, m=4, exit program
	halt	$zero,	$zero,	$zero,	0	# pc15	n=4, m=4, exit program
	halt	$zero,	$zero,	$zero,	0	# pc16	n=4, m=4, exit program

Calc:	add	$t0,	$zero,	$zero,	0	# pc17	reset i
	add	$v0,	$zero,	$zero,	0	# pc18	reset result
iLoop:	mul	$t1,	$t0,	$sp,	0	# pc19	t1=i*16
	add	$t1,	$t1,	$a0,	0	# pc1A	t1+=n
	add	$t1,	$t1,	$s0,	0	# pc1B	addr of A(n,i)
	mul	$t2,	$a1,	$sp,	0	# pc1C	t2=m*16
	add	$t2,	$t2,	$t0,	0	# pc1D	t2+=i
	add	$t2,	$t2,	$s1,	0	# pc1E	addr of B(i,m)
	lw	$t1,	$t1,	$zero,	0	# pc1F	A(n,i)=mem[]
	lw	$t2,	$t2,	$zero,	0	# pc20	B(i,m)=mem[]
	mul	$t1,	$t1,	$t2,	0	# pc21	A(n,i)*B(i,m)
	add	$v0,	$v0,	$t1,	0	# pc22	update result

	add	$t0,	$t0,	$fp,	0	# pc23	i++
	add	$t1,	$zero,	$sp,	0	# pc24	t1=16
	blt	$imm,	$t0,	$t1,	iLoop	# pc25	if (i<16) go to iLoop
	add	$zero,	$zero,	$zero,	0	# pc26	(delay slot)

	mul	$t1,	$a1,	$sp,	0	# pc27	t1=m*16
	add	$t2,	$a0,	$zero,	0	# pc28	t2=n {0-15}
	add	$t2,	$t2,	$t1,	0	# pc29	t2=m*16+n
	add	$t2,	$s2,	$t2,	0	# pc2A	addr of C(n,m)
	sw	$v0,	$t2,	$zero,	0	# pc2B	mem[m*16+n]=C(n,m)

	beq	$imm,	$zero,	$zero,	Next	# pc2C	return to n,m Loops
	add	$zero,	$zero,	$zero,	0	# pc2D	(delay slot)
