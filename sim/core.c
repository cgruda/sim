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

#define PC_MASK		0x000003FF
#define INVALID_PC	0xFFFFFFFF
#define REG_MASK	0x0000000F

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

#define INST_OP_GET(inst)	(((inst) & (INST_OP_MSK)) >> (INST_OP_OFT))
#define INST_RD_GET(inst)	(((inst) & (INST_RD_MSK)) >> (INST_RD_OFT))
#define INST_RS_GET(inst)	(((inst) & (INST_RS_MSK)) >> (INST_RS_OFT))
#define INST_RT_GET(inst)	(((inst) & (INST_RT_MSK)) >> (INST_RT_OFT))
#define INST_IM_GET(inst)	(((inst) & (INST_IM_MSK)) >> (INST_IM_OFT))

void core_branch_resolution(struct core *p_core, uint8_t op, uint8_t rtv, uint8_t rsv, uint8_t rdv)
{
	struct pipe *id_ex = &p_core->pipe[ID_EX];
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
		id_ex->dst.d = 15;
		id_ex->alu.d = p_core->pc.q + 1;
		// reg[15].d = p_core->pc.q + 1;
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

	if (p_core->stall) {
		dbg_verbose("[Core%d][FETCH] STALL\n", p_core->idx);
		return;
	}

	if_id->ir.d = imem[pc & PC_MASK];
	if_id->npc.d = pc;

	dbg_verbose("[Core%d][FETCH] pc=%d (inst=%08x)\n", p_core->idx, pc, if_id->ir.d);
}

bool core_read_after_write_hazard(struct core *p_core, uint32_t id_inst)
{
	bool hazard_rd = false;
	bool hazard_rs = false;
	bool hazard_rt = false;
	bool hazard = false;
	struct pipe *p_pipe;

	uint8_t op = INST_OP_GET(id_inst);
	uint8_t rd = INST_RD_GET(id_inst);
	uint8_t rs = INST_RS_GET(id_inst);
	uint8_t rt = INST_RT_GET(id_inst);

	// TODO: since you wait for WB you actually have an extera cycle.
	// this seems to be correct for now, based on trace files given in
	// the site. needs to be tested though.
	for (int i = ID_EX; i < PIPE_MAX; i++) {
		p_pipe = &p_core->pipe[i];
		
		// dst 0 means there is for sure no hazard
		if (!p_pipe->dst.q)
			continue;

		hazard_rd = rd && (rd == p_pipe->dst.q);
		hazard_rs = rs && (rs == p_pipe->dst.q);
		hazard_rt = rt && (rt == p_pipe->dst.q);

		if (op == OP_HALT) {
			break;
		} else if (op == OP_JAL) {
			hazard = hazard_rd;
		} else {
			hazard = hazard_rs || hazard_rt;
			if (op >= OP_BEQ && op <= OP_BGE || op == OP_SW)
				hazard |= hazard_rd;
		}

		if (hazard) {
			dbg_verbose("[Core%d][DEC] RAW Hazard: inst=%08x (pc=%d). [D%d;S%d;T%d][pipe%d;dst=%x]\n", p_core->idx, id_inst, p_core->pipe[IF_ID].npc.q, hazard_rd,
				hazard_rs, hazard_rt, i, p_pipe->dst.q);
			break;
		}
	}

	return hazard;
}

void core_stall(struct core *p_core)
{
	struct pipe *id_ex = &p_core->pipe[ID_EX];

	id_ex->dst.d = 0;
	id_ex->npc.d = INVALID_PC;

	p_core->stall = true;
}

void core_unstall(struct core *p_core)
{
	p_core->stall = false;
}

void core_decode_instruction(struct core *p_core)
{
	uint32_t sign_extention_mask = 0;
	uint32_t sign_extended_imm = 0;
	struct pipe *if_id = &p_core->pipe[IF_ID];
	struct pipe *id_ex = &p_core->pipe[ID_EX];
	reg32_t *reg = p_core->reg;

	// decode instruction
	uint32_t inst = if_id->ir.q;
	uint8_t op = INST_OP_GET(inst);
	uint8_t rd = INST_RD_GET(inst);
	uint8_t rs = INST_RS_GET(inst);
	uint8_t rt = INST_RT_GET(inst);
	uint16_t im = INST_IM_GET(inst);

	if (if_id->npc.q == INVALID_PC) {
		id_ex->npc.d = if_id->npc.q;
		if (!p_core->halt)
			p_core->pc.d = p_core->pc.q + 1;
		return;
	}

	if (core_read_after_write_hazard(p_core, inst)) {
		core_stall(p_core);
		return;
	} else {
		core_unstall(p_core);
	}

	// sign extend and store imm value in reg1 (D and Q)
	if (im & BIT(11))
		sign_extention_mask = ~(BIT(12) - 1);
	p_core->reg[1].d = im | sign_extention_mask;
	p_core->reg[1].q = p_core->reg[1].d;

	dbg_verbose("[Core%d][DEC] inst=%08x (pc=%d), imm=%x\n", p_core->idx, inst, if_id->npc.q, p_core->reg[1].d);

	// next pipe stage set
	id_ex->npc.d = if_id->npc.q;
	id_ex->rtv.d = reg[rt & REG_MASK].q;
	id_ex->rsv.d = reg[rs & REG_MASK].q;
	id_ex->rdv.d = reg[rd & REG_MASK].q;
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
	uint32_t alu = 0;

	if (id_ex->npc.q == INVALID_PC) {
		ex_mem->npc.d = id_ex->npc.q;
		ex_mem->dst.d = id_ex->dst.q;
		return;
	}

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
	case OP_JAL:
		alu = id_ex->alu.q;
		break;
	case OP_LW:
		alu = rsv + rtv;
		break;
	case OP_SW:
		alu = rsv + rtv;
		break;
	case OP_HALT:
		p_core->halt = true;
		p_core->pc.d = INVALID_PC;
		p_core->pipe[IF_ID].npc.d = INVALID_PC;
		p_core->pipe[IF_ID].npc.q = INVALID_PC;
		id_ex->npc.d = INVALID_PC;
		break;
	default:
		break;
	}

	dbg_verbose("[Core%d][EXEC] op=%d (pc=%d), alu=%d\n", p_core->idx, op, id_ex->npc.q, alu);

	// next pipe stage set
	ex_mem->npc.d = id_ex->npc.q;
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

	if (ex_mem->npc.q == INVALID_PC) {
		mem_wb->npc.d = ex_mem->npc.q;
		mem_wb->dst.d = ex_mem->dst.q;
		return;
	}

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

	dbg_verbose("[Core%d][MEM] op=%d (pc=%d), addr=%08x data=%08x\n", p_core->idx, op, ex_mem->npc.q, addr, data);

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
	uint8_t dst = 0;
	uint8_t val = 0;

	if (mem_wb->npc.q == INVALID_PC)
		return;

	if (op < OP_BEQ || op == OP_JAL) {
		dst = mem_wb->dst.q & REG_MASK;
		val = mem_wb->alu.q;
	} else if (op == OP_LW) {
		dst = mem_wb->dst.q & REG_MASK;
		val = mem_wb->md.q;
	}

	reg[dst].d = val;

	dbg_verbose("[Core%d][WB] op=%d (pc=%d), dst=%d val=%08x\n", p_core->idx, op, mem_wb->npc.q, dst, val);
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

	p_core->idx = idx;

	return p_core;
}

int core_load(char **file_paths, struct core *p_core, uint32_t *main_mem)
{
	int res = 0;

	p_core->trace_path = file_paths[PATH_CORE0TRACE + p_core->idx];
	p_core->mem = main_mem; // FIXME: temp

	res = mem_load(file_paths[PATH_IMEME0 + p_core->idx], p_core->imem, IMEM_LEN);
	if (res < 0)
		return -1;

	for (int i = 0; i < PIPE_MAX; i++) {
		p_core->pipe[i].npc.q = INVALID_PC;
		p_core->pipe[i].npc.d = INVALID_PC;
	}

	dbg_info("Core%d loaded\n", p_core->idx);

	return 0;
}

void core_trace_pipe(FILE *fp, struct core *p_core)
{
	uint32_t pc;

	pc = p_core->pipe[IF_ID].npc.d;
	if (pc != INVALID_PC)
		fprintf(fp, "%03d ", pc);
	else
		fprintf(fp, "--- ");


	for (int i = ID_EX; i < PIPE_MAX + 1; i++) {
		pc = p_core->pipe[i - 1].npc.q;
		if (pc != INVALID_PC)
			fprintf(fp, "%03d ", pc);
		else
			fprintf(fp, "--- ");
	}
}

void core_trace_reg(FILE *fp, struct core *p_core)
{
	reg32_t *reg = p_core->reg;

	for (int i = 2; i < REG_MAX; i++)
		fprintf(fp, "%08X ", reg[i].q);
}

int core_trace(struct core *p_core)
{
	FILE *fp = NULL;

	if (!(fp = fopen(p_core->trace_path, "a"))) {
		print_error("Failed to open \"%s\"", p_core->trace_path);
		return -1;
	}

	fprintf(fp, "%d ", g_clk);
	core_trace_pipe(fp, p_core);
	core_trace_reg(fp, p_core);
	fprintf(fp, "\n");

	fclose(fp);
	return 0;
}

void core_stats(struct core *p_core)
{
	// TODO:
}

void core_cycle(struct core *p_core)
{
	// Core functionality
	core_fetch_instruction(p_core);
	core_decode_instruction(p_core);
	core_execute_instruction(p_core);
	core_memory_access(p_core);
	core_write_back(p_core);

	// Core race and stats
	core_trace(p_core);
	core_stats(p_core);
}

void core_clock_tick(struct core *p_core)
{
	reg32_t *reg = p_core->reg;
	struct pipe *pipe;

	for (int i = 0; i < REG_MAX; i++)
		reg[i].q = reg[i].d;
	
	for (int i = 0; i < PIPE_MAX; i++) {
		if (p_core->stall && i == IF_ID)
			continue;

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

bool core_is_halt(struct core *p_core)
{
	return p_core->halt;
}

bool core_is_done(struct core *p_core)
{
	if (p_core->done)
		return true;

	if (core_is_halt(p_core)) {
		for (int i = 0; i < PIPE_MAX; i++) {
			if (p_core->pipe[i].npc.q != INVALID_PC)
				return false;
		}

		dbg_info("Core%d is done. g_clk=%d\n", p_core->idx, g_clk);
		p_core->done = true;
	}

	return p_core->done;
}

int core_dump_reg(char *path, struct core *p_core)
{
	FILE *fp = NULL;

	if (!(fp = fopen(path, "w"))) {
		print_error("Failed to open \"%s\"", path);
		return -1;
	}

	for (int i = 2; i < REG_MAX; i++)
		fprintf(fp, "%08x\n", p_core->reg[i].q);

	fclose(fp);
	return 0;
}