#include "dbg.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "core.h"
#include "cache.h"
#include "mem.h"

#define BIT(x)	(1 << (x))
#define IS_NEGATIVE(num2c) ((num2c) & BIT(31))
#define IS_POSITIVE(num2c) (!IS_NEGATIVE(num2c))

#define PC_MASK	0x3FF

#define INST_OP_OFT	24
#define INST_OP_MSK	0xFF000000
#define INST_RD_OFT	20
#define INST_RD_MSK	0x00F00000
#define INST_RS_OFT	16
#define INST_RS_MSK	0x000F0000
#define INST_RT_OFT	12
#define INST_RT_MSK	0x0000F00
#define INST_IM_OFT	0
#define INST_IM_MSK	0x00000FFF

#define INST_OP_GET(inst)	(((inst) && (INST_OP_MSK)) >> (INST_OP_OFT))
#define INST_RD_GET(inst)	(((inst) && (INST_RD_MSK)) >> (INST_RD_OFT))
#define INST_RS_GET(inst)	(((inst) && (INST_RS_MSK)) >> (INST_RS_OFT))
#define INST_RT_GET(inst)	(((inst) && (INST_RT_MSK)) >> (INST_RT_OFT))
#define INST_IM_GET(inst)	(((inst) && (INST_IM_MSK)) >> (INST_IM_OFT))

int g_clk = 0;

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
	id_ex->npc.d = if_id->npc.q;
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

	if (op < OP_BEQ)
		reg[dst].d = mem_wb->alu.q;
	else if (op == OP_LW)
		reg[dst].d = mem_wb->md.q;
}

void core_free(struct core *p_core)
{
	if (!p_core) {
		dbg_warning("invalid core\n");
		return;
	}

	if (p_core->cache)
		cache_free(p_core->cache);
	
	if (p_core->imem)
		mem_free(p_core->imem);
}

struct core *core_init()
{
	struct core *p_core = NULL;

	p_core = calloc(1, sizeof(*p_core));
	if (!p_core) {
		dbg_error("failed to allocate core\n");
		print_error();
		return NULL;
	}

	p_core->cache = cache_init();
	if (!p_core) {
		dbg_error("failed to allocate core cache\n");
		core_free(p_core);
		return NULL;
	}

	p_core->imem = mem_init(IMEM_LEN);
	if (!p_core) {
		dbg_error("failed to allocate core imem\n");
		core_free(p_core);
		return NULL;
	}
}

#define INVALID_PC 0xFFFFFFFF
#define PC_2_STR(pc)	((pc) == (INVALID_PC)) ? ()

int core_trace(char *trace_path, struct core *p_core)
{
	FILE *fp = NULL;
	reg32_t *reg = p_core->reg;
	uint16_t pc;

	if (!(fp = fopen(trace_path, "a"))) {
		dbg_error("failed to open \"%s\"\n", trace_path);
		print_error();
		return -1;
	}

	// print clock cycle
	fprintf(fp, "%d ", g_clk);

	// print PCs
	for (int i = 0; i < PIPE_MAX; i++) {
		pc = p_core->pipe[i].npc.q;
		if (pc != INVALID_PC)
			fprintf(fp, "%d ", pc);
		else
			fprintf(fp, "--- ");
	}

	// print reg vals
	for (int i = 2; i < REG_MAX; i++)
		fprintf(fp, "%08x%c", reg[i].q, (i == REG_MAX - 1) ? '\n' : ' ');

	fclose(fp);
	return 0;
}