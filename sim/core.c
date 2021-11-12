#include "dbg.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "common.h"
#include "core.h"
#include "cache.h"
#include "mem.h"

#define IS_NEGATIVE(num2c) ((num2c) & BIT(31))
#define IS_POSITIVE(num2c) (!IS_NEGATIVE(num2c))

#define PC_MASK		0x3FF
#define INVALID_PC	0xFFFFFFFF

#define INST_OP_OFT	24
#define INST_OP_MSK	0xFF000000
#define INST_RD_OFT	20
#define INST_RD_MSK	0x00F00000
#define INST_RS_OFT	16
#define INST_RS_MSK	0x000F0000
#define INST_RT_OFT	12
#define INST_RT_MSK	0x0000F000
#define INST_IM_OFT	0
#define INST_IM_MSK	0x00000FFF

#define INST_OP_GET(inst)	(((inst) && (INST_OP_MSK)) >> (INST_OP_OFT))
#define INST_RD_GET(inst)	(((inst) && (INST_RD_MSK)) >> (INST_RD_OFT))
#define INST_RS_GET(inst)	(((inst) && (INST_RS_MSK)) >> (INST_RS_OFT))
#define INST_RT_GET(inst)	(((inst) && (INST_RT_MSK)) >> (INST_RT_OFT))
#define INST_IM_GET(inst)	(((inst) && (INST_IM_MSK)) >> (INST_IM_OFT))

void core_branch_resolution(struct core *p_core, uint8_t op, uint8_t rtv, uint8_t rsv, uint8_t rdv)
{
	reg32_t *reg = p_core->reg;
	bool branch = false;
	
	switch (op) {
	case OP_BEQ:
		branch = rsv == rtv;
		break;
	case OP_BNE:
		branch = rsv != rtv;
		break;
	case OP_BLT:
		if (IS_NEGATIVE(rsv) && IS_NEGATIVE(rtv))
			branch = rsv > rtv;
		else if (IS_POSITIVE(rsv) && IS_POSITIVE(rtv))
			branch = rsv < rtv;
		else if (IS_NEGATIVE(rsv) && IS_POSITIVE(rtv))
			branch = true;
		break;
	case OP_BGT:
		if (IS_NEGATIVE(rsv) && IS_NEGATIVE(rtv))
			branch = rsv < rtv;
		else if (IS_POSITIVE(rsv) && IS_POSITIVE(rtv))
			branch = rsv > rtv;
		else if (IS_POSITIVE(rsv) && IS_NEGATIVE(rtv))
			branch = true;
		break;
	case OP_BLE:
		if (IS_NEGATIVE(rsv) && IS_NEGATIVE(rtv))
			branch = rsv >= rtv;
		else if (IS_POSITIVE(rsv) && IS_POSITIVE(rtv))
			branch = rsv <= rtv;
		else if (IS_NEGATIVE(rsv) && IS_POSITIVE(rtv))
			branch = true;
		break;
	case OP_BGE:
		if (IS_NEGATIVE(rsv) && IS_NEGATIVE(rtv))
			branch = rsv <= rtv;
		else if (IS_POSITIVE(rsv) && IS_POSITIVE(rtv))
			branch = rsv >= rtv;
		else if (IS_POSITIVE(rsv) && IS_NEGATIVE(rtv))
			branch = true;
		break;
	case OP_JAL:
		reg[15].d = p_core->pc.q + 1;
		branch = true;
		break;
	default:
		branch = false;
		break;
	}

	if (branch)
		p_core->pc.d = rdv & PC_MASK;
	else
		p_core->pc.d = p_core->pc.q + 1;
}

void core_fetch_instruction(struct core *p_core)
{
	struct pipe *if_id = &p_core->pipe[IF_ID];
	uint32_t *imem = p_core->imem;
	uint32_t pc = p_core->pc.q;

	if_id->ir.d = imem[pc];
	if_id->npc.d = pc;
}

void core_decode_instruction(struct core *p_core)
{
	uint32_t sign_extention_mask = 0;
	uint32_t sign_extended_imm = 0;
	struct pipe *if_id = &p_core->pipe[IF_ID];
	struct pipe *id_ex = &p_core->pipe[ID_EX];
	reg32_t *reg = p_core->reg;

	id_ex->npc.d = if_id->npc.q;
	if (if_id->npc.q == INVALID_PC) {
		p_core->pc.d = p_core->pc.q + 1;
		return;
	}

	// decode instruction
	uint32_t inst = if_id->ir.q;
	uint8_t op = INST_OP_GET(inst);
	uint8_t rd = INST_RD_GET(inst);
	uint8_t rs = INST_RS_GET(inst);
	uint8_t rt = INST_RT_GET(inst);
	uint16_t im = INST_IM_GET(inst);

	// sign extend and store imm value
	if (im & BIT(11))
		sign_extention_mask = ~(BIT(12) - 1);
	p_core->reg[1].d = im | sign_extention_mask;

	// next pipe stage set
	id_ex->rtv.d = reg[rt].q;
	id_ex->rsv.d = reg[rs].q;
	id_ex->rdv.d = reg[rd].q;
	id_ex->dst.d = rd;
	id_ex->ir.d = if_id->ir.q;

	core_branch_resolution(p_core, op, id_ex->rtv.d, id_ex->rsv.d, id_ex->rdv.d);
}

void core_execute_instruction(struct core *p_core)
{
	struct pipe *id_ex = &p_core->pipe[ID_EX];
	struct pipe *ex_mem = &p_core->pipe[EX_MEM];
	uint32_t rsv = id_ex->rsv.q;
	uint32_t rtv = id_ex->rtv.q;
	uint32_t op = INST_OP_GET(id_ex->ir.q);
	uint32_t alu;

	ex_mem->npc.d = id_ex->npc.q;
	if (id_ex->npc.q == INVALID_PC)
		return;

	uint32_t sign_extention_mask = 0;
	if (IS_NEGATIVE(rsv))
		sign_extention_mask = ~(BIT(31 - rtv) - 1);

	switch (op) {
	case OP_ADD:
		alu = rsv + rtv;
		break;
	case OP_SUB:
		alu = rsv - rtv;
		break;
	case OP_AND:
		alu = rsv & rtv;
		break;
	case OP_OR:
		alu = rsv | rtv;
		break;
	case OP_XOR:
		alu = rsv ^ rtv;
		break;
	case OP_MUL:
		alu = rsv * rtv;
		break;
	case OP_SLL:
		alu = rsv << rtv;
		break;
	case OP_SRA:
		alu = (rsv >> rtv) | sign_extention_mask;
		break;
	case OP_SRL:
		alu = rsv >> rtv;
		break;
	case OP_LW:
		alu = rsv + rtv;
		break;
	case OP_SW:
		alu = rsv + rtv;
		break;
	case OP_HALT:
		p_core->halt = true;
		break;
	default:
		break;
	}

	// next pipe stage set
	ex_mem->alu.d = alu;
	ex_mem->npc.d = id_ex->npc.q;
	ex_mem->rdv.d = id_ex->rdv.q;
	ex_mem->dst.d = id_ex->dst.q;
	ex_mem->ir.d = id_ex->ir.q;
}

void core_memory_access(struct core *p_core)
{
	struct pipe *ex_mem = &p_core->pipe[EX_MEM];
	struct pipe *mem_wb = &p_core->pipe[MEM_WB];
	uint32_t op = INST_OP_GET(ex_mem->ir.q);
	uint32_t *mem = p_core->mem;
	uint32_t addr = ex_mem->alu.q;
	uint32_t data = 0;

	mem_wb->npc.d = ex_mem->npc.q;
	if (ex_mem->npc.q == INVALID_PC)
		return;

	// access main memory // TODO: this is remporary direct access
	switch(op) {
	case OP_LW:
		data = mem_read(mem, addr);
		break;
	case OP_SW:
		data = ex_mem->rdv.q;
		mem_write(mem, addr, data);
		break;
	default:
		break;
	}

	// next pipe stage set
	mem_wb->npc.d = ex_mem->npc.q;
	mem_wb->alu.d = ex_mem->alu.q;
	mem_wb->dst.d = ex_mem->dst.q;
	mem_wb->ir.d = ex_mem->ir.q;
	mem_wb->md.d = data;
}

void core_write_back(struct core *p_core)
{
	reg32_t *reg = p_core->reg;
	struct pipe *mem_wb = &p_core->pipe[MEM_WB];
	uint32_t op = INST_OP_GET(mem_wb->ir.q);
	uint8_t dst = mem_wb->dst.q;

	if (mem_wb->npc.q == INVALID_PC)
		return;

	if (op < OP_BEQ)
		reg[dst].d = mem_wb->alu.q;
	else if (op == OP_LW)
		reg[dst].d = mem_wb->md.q;
}

void core_free(struct core *p_core)
{
	if (!p_core) {
		dbg_warning("Invalid core\n");
		return;
	}

	if (p_core->cache)
		cache_free(p_core->cache);
	
	if (p_core->imem)
		mem_free(p_core->imem);

	// TODO: temporary
	if (p_core->mem)
		mem_free(p_core->mem);
}

struct core *core_init(int idx)
{
	struct core *p_core = NULL;

	if (idx >= CORE_MAX) {
		dbg_error("Invalid core_idx=%d, max cores allowed is %d\n", idx, CORE_MAX);
		return NULL;
	}

	p_core = calloc(1, sizeof(*p_core));
	if (!p_core) {
		print_error("Failed to allocate core");
		return NULL;
	}

	p_core->cache = cache_init();
	if (!p_core->cache) {
		dbg_error("Failed to allocate core cache\n");
		core_free(p_core);
		return NULL;
	}

	p_core->imem = mem_init(IMEM_LEN);
	if (!p_core->imem) {
		dbg_error("Failed to allocate core imem\n");
		core_free(p_core);
		return NULL;
	}

	// TODO: temporary
	p_core->mem = mem_init(MEM_LEN);
	if (!p_core->mem) {
		dbg_error("Failed to allocate core main mem\n");
		core_free(p_core);
		return NULL;
	}

	p_core->idx = idx;

	return p_core;
}

int core_load(char **file_paths, struct core *p_core, uint32_t *main_mem)
{
	int res = 0;

	p_core->trace_path = file_paths[PATH_CORE0TRACE + p_core->idx];
	p_core->mem = main_mem;

	res = mem_load(file_paths[PATH_IMEME0 + p_core->idx], p_core->imem, IMEM_LEN);
	if (res < 0)
		return -1;

	for (int i = 0; i < PIPE_MAX; i++) {
		p_core->pipe[i].npc.q = INVALID_PC;
		p_core->pipe[i].npc.d = INVALID_PC;
	}

	dbg_verbose("Core %d loaded\n", p_core->idx);

	return 0;
}

int core_trace(struct core *p_core)
{
	FILE *fp = NULL;
	reg32_t *reg = p_core->reg;
	uint32_t pc;

	if (!(fp = fopen(p_core->trace_path, "a"))) {
		print_error("Failed to open \"%s\"", p_core->trace_path);
		return -1;
	}

	fprintf(fp, "%2d ", g_clk); // FIXME: remove print indent

	// Print IF, ID, EX and MEM PCs
	for (int i = 0; i < PIPE_MAX; i++) {
		pc = p_core->pipe[i].npc.d; // D was just done by the stage, and set into the pipe
		if (pc != INVALID_PC)
			fprintf(fp, "%3d ", pc); // FIXME: remove print indent
		else
			fprintf(fp, "--- ");
	}

	// Print WB PC
	pc = p_core->pipe[MEM_WB].npc.q; // q of MEM_WB was just done by WB (and lost)
	if (pc != INVALID_PC)
		fprintf(fp, "%3d ", pc); // FIXME: remove print indent
	else
		fprintf(fp, "--- ");

	for (int i = 2; i < REG_MAX; i++)
		fprintf(fp, "%08x%c", reg[i].q, (i == REG_MAX - 1) ? '\n' : ' ');

	fclose(fp);
	return 0;
}

void core_cycle(struct core *p_core)
{
	core_fetch_instruction(p_core);
	core_decode_instruction(p_core);
	core_execute_instruction(p_core);
	core_memory_access(p_core);
	core_write_back(p_core);
	core_trace(p_core);
	// TODO: core stats
}

void core_clock_tick(struct core *p_core)
{
	reg32_t *reg = p_core->reg;
	struct pipe *pipe;

	for (int i = 0; i < REG_MAX; i++)
		reg[i].q = reg[i].d;
	
	for (int i = 0; i < PIPE_MAX; i++) {
		pipe = &p_core->pipe[i];
		pipe->npc.q = pipe->npc.d;
		pipe->rsv.q = pipe->rsv.d;
		pipe->rtv.q = pipe->rtv.d;
		pipe->rdv.q = pipe->rdv.d;
		pipe->alu.q = pipe->alu.d;
		pipe->dst.q = pipe->dst.d;
		pipe->md.q = pipe->md.d;
		pipe->ir.q = pipe->ir.d;
	}

	p_core->pc.q = p_core->pc.d;
}