#include <stdint.h>
#include <stdbool.h>
#include "core.h"
#include "cache.h"

#define BIT(pos)	(1 << (pos))
#define IS_NEGATIVE(num2c) ((num2c) & BIT(31))
#define IS_POSITIVE(num2c) (!IS_NEGATIVE(num2c))

void add(uint32_t *pc, struct cache *MEM, uint32_t *R, uint32_t rd, uint32_t rs, uint32_t rt)
{
	R[rd] = R[rs] + R[rt];
	(*pc)++;
}

void sub(uint32_t *pc, struct cache *MEM, uint32_t *R, uint32_t rd, uint32_t rs, uint32_t rt)
{
	R[rd] = R[rs] - R[rt];
	(*pc)++;
}

void and(uint32_t *pc, struct cache *MEM, uint32_t *R, uint32_t rd, uint32_t rs, uint32_t rt)
{
	R[rd] = R[rs] & R[rt];
	(*pc)++;
}

void or(uint32_t *pc, struct cache *MEM, uint32_t *R, uint32_t rd, uint32_t rs, uint32_t rt)
{
	R[rd] = R[rs] | R[rt];
	(*pc)++;
}

void xor(uint32_t *pc, struct cache *MEM, uint32_t *R, uint32_t rd, uint32_t rs, uint32_t rt)
{
	R[rd] = R[rs] ^ R[rt];
	(*pc)++;
}


void mul(uint32_t *pc, struct cache *MEM, uint32_t *R, uint32_t rd, uint32_t rs, uint32_t rt)
{
	R[rd] = R[rs] * R[rt];
	(*pc)++;
}

void sll(uint32_t *pc, struct cache *MEM, uint32_t *R, uint32_t rd, uint32_t rs, uint32_t rt)
{
	R[rd] = R[rs] << R[rt];
	(*pc)++;
}

void sra(uint32_t *pc, struct cache *MEM, uint32_t *R, uint32_t rd, uint32_t rs, uint32_t rt)
{
	uint32_t sign_extention_mask = 0;

	if (IS_NEGATIVE(R[rs]))
		sign_extention_mask = ~(BIT(31 - R[rt]) - 1);

	R[rd] = (R[rs] >> R[rt]) | sign_extention_mask;
	(*pc)++;
}

void srl(uint32_t *pc, struct cache *MEM, uint32_t *R, uint32_t rd, uint32_t rs, uint32_t rt)
{
	R[rd] = R[rs] >> R[rt];
	(*pc)++;
}

void beq(uint32_t *pc, struct cache *MEM, uint32_t *R, uint32_t rd, uint32_t rs, uint32_t rt)
{
	if (R[rs] == R[rt])
		*pc = R[rd];
	else
		(*pc)++;
}

void bne(uint32_t *pc, struct cache *MEM, uint32_t *R, uint32_t rd, uint32_t rs, uint32_t rt)
{
	if (R[rs] != R[rt])
		*pc = R[rd];
	else
		(*pc)++;
}

void blt(uint32_t *pc, struct cache *MEM, uint32_t *R, uint32_t rd, uint32_t rs, uint32_t rt)
{
	bool branch = false;

	if (IS_NEGATIVE(R[rs]) && IS_NEGATIVE(R[rt]))
		branch = R[rs] > R[rt];
	else if (IS_POSITIVE(R[rs]) && IS_POSITIVE(R[rt]))
		branch = R[rs] < R[rt];
	else if (IS_NEGATIVE(R[rs]) && IS_POSITIVE(R[rt]))
		branch = true;
	else if (IS_POSITIVE(R[rs]) && IS_NEGATIVE(R[rt]))
		branch = false;

	if (branch)
		*pc = R[rd];
	else
		(*pc)++;
}

void bgt(uint32_t *pc, struct cache *MEM, uint32_t *R, uint32_t rd, uint32_t rs, uint32_t rt)
{
	bool branch = false;

	if (IS_NEGATIVE(R[rs]) && IS_NEGATIVE(R[rt]))
		branch = R[rs] < R[rt];
	else if (IS_POSITIVE(R[rs]) && IS_POSITIVE(R[rt]))
		branch = R[rs] > R[rt];
	else if (IS_NEGATIVE(R[rs]) && IS_POSITIVE(R[rt]))
		branch = false;
	else if (IS_POSITIVE(R[rs]) && IS_NEGATIVE(R[rt]))
		branch = true;

	if (branch)
		*pc = R[rd];
	else
		(*pc)++;
}

void ble(uint32_t *pc, struct cache *MEM, uint32_t *R, uint32_t rd, uint32_t rs, uint32_t rt)
{
	bool branch = false;

	if (IS_NEGATIVE(R[rs]) && IS_NEGATIVE(R[rt]))
		branch = R[rs] >= R[rt];
	else if (IS_POSITIVE(R[rs]) && IS_POSITIVE(R[rt]))
		branch = R[rs] <= R[rt];
	else if (IS_NEGATIVE(R[rs]) && IS_POSITIVE(R[rt]))
		branch = true;
	else if (IS_POSITIVE(R[rs]) && IS_NEGATIVE(R[rt]))
		branch = false;

	if (branch)
		*pc = R[rd];
	else
		(*pc)++;
}

void bge(uint32_t *pc, struct cache *MEM, uint32_t *R, uint32_t rd, uint32_t rs, uint32_t rt)
{
	bool branch = false;

	if (IS_NEGATIVE(R[rs]) && IS_NEGATIVE(R[rt]))
		branch = R[rs] <= R[rt];
	else if (IS_POSITIVE(R[rs]) && IS_POSITIVE(R[rt]))
		branch = R[rs] >= R[rt];
	else if (IS_NEGATIVE(R[rs]) && IS_POSITIVE(R[rt]))
		branch = false;
	else if (IS_POSITIVE(R[rs]) && IS_NEGATIVE(R[rt]))
		branch = true;

	if (branch)
		*pc = R[rd];
	else
		(*pc)++;
}

void jal(uint32_t *pc, struct cache *MEM, uint32_t *R, uint32_t rd, uint32_t rs, uint32_t rt)
{
	R[15] = *pc + 1;
	*pc = R[rd];
}

void lw(uint32_t *pc, struct cache *MEM, uint32_t *R, uint32_t rd, uint32_t rs, uint32_t rt)
{
	// R[rd] = MEM[R[rs] + R[rt]]; // FIXME:
	(*pc)++;
}

void sw(uint32_t *pc, struct cache *MEM, uint32_t *R, uint32_t rd, uint32_t rs, uint32_t rt)
{
	// MEM[R[rs] + R[rt]] = R[rd]; // FIXME:
	(*pc)++;
}

void halt(uint32_t *pc, struct cache *MEM, uint32_t *R, uint32_t rd, uint32_t rs, uint32_t rt)
{
	
}

int(*core_op[OP_MAX])(uint32_t *pc, struct cache *MEM, uint32_t *R, uint32_t rd, uint32_t rs, uint32_t rt) =
{
	[OP_ADD]  = add,
	[OP_SUB]  = sub,
	[OP_AND]  = and,
	[OP_OR]   = or,
	[OP_XOR]  = xor,
	[OP_MUL]  = mul,
	[OP_SLL]  = sll,
	[OP_SRA]  = sra,
	[OP_SRL]  = srl,
	[OP_BEQ]  = beq,
	[OP_BNE]  = bne,
	[OP_BLT]  = blt,
	[OP_BGT]  = bgt,
	[OP_BLE]  = ble,
	[OP_BGE]  = bge,
	[OP_JAL]  = jal,
	[OP_LW]   = lw,
	[OP_SW]   = sw,
	[OP_HALT] = halt,
};

void core_execute(struct core *p_core, struct inst *p_inst)
{

}
