#include "dbg.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "common.h"
#include "core.h"
#include "cache.h"
#include "mem.h"
#include "bus.h"

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

char *core_stat_name_2_str[STATS_MAX] = {
	[STATS_CYCLE]        = "cycles",
	[STATS_INSTRUCTION]  = "instructions",
	[STATS_READ_HIT]     = "read_hit",
	[STATS_WRITE_HIT]    = "write_hit",
	[STATS_READ_MISS]    = "read_miss",
	[STATS_WRITE_MISS]   = "write_miss",
	[STATS_DECODE_STALL] = "decode_stall",
	[STATS_MEM_STALL]    = "mem_stall",
};

void core_branch_resolution(struct core *p_core, uint8_t op, uint8_t rtv, uint8_t rsv, uint8_t rdv)
{
	struct pipe *id_ex = &p_core->pipe[ID_EX];
	reg32_t *reg = p_core->reg;
	bool branch = false;
	
	// TODO: delay slot??
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
		branch = true;
		break;

	default:
		branch = false;
		break;
	}

	if (branch) {
		p_core->pc.d = rdv & PC_MASK;
	} else {
		p_core->pc.d = p_core->pc.q + 1;
	}
}

bool core_hazard_raw(struct core *p_core, uint32_t id_inst, uint16_t *raw_info)
{
	bool hazard = false;
	bool hazard_rd = false;
	bool hazard_rs = false;
	bool hazard_rt = false;
	struct pipe *p_pipe;

	uint8_t op = INST_OP_GET(id_inst);
	uint8_t rd = INST_RD_GET(id_inst);
	uint8_t rs = INST_RS_GET(id_inst);
	uint8_t rt = INST_RT_GET(id_inst);

	if (op == OP_HALT) {
		return false;
	}

	for (int i = ID_EX; i < PIPE_MAX; i++) {
		p_pipe = &p_core->pipe[i];

		if (!p_pipe->dst.q) {
			continue;
		}

		hazard_rd = (rd > 1) && (rd == p_pipe->dst.q);
		hazard_rs = (rs > 1) && (rs == p_pipe->dst.q);
		hazard_rt = (rt > 1) && (rt == p_pipe->dst.q);

		if (op != OP_JAL) {
			hazard |= hazard_rs || hazard_rt;
		}

		if ((op >= OP_BEQ && op <= OP_BGE) || op == OP_JAL || op == OP_SW) {
			hazard |= hazard_rd;
		}

		if (hazard) {
			break;
		}
	}

	*raw_info = (hazard_rd ? (p_pipe->dst.q << 12) : 0) |
		    (hazard_rs ? (p_pipe->dst.q << 8)  : 0) |
		    (hazard_rt ? (p_pipe->dst.q << 4)  : 0);

	return hazard;
}

void core_stall(struct core *p_core, uint8_t stage)
{
	struct pipe *id_ex = &p_core->pipe[ID_EX];
	struct pipe *ex_mem = &p_core->pipe[EX_MEM];
	struct pipe *mem_wb = &p_core->pipe[MEM_WB];

	switch(stage) {
	case ID_EX:
		id_ex->dst.d = 0;
		id_ex->npc.d = INVALID_PC;
		p_core->stall_decode = true;
		break;

	case MEM_WB:
		mem_wb->npc.d = INVALID_PC;
		p_core->stall_mem = true;
		break;
	}
}

void core_write(struct core *p_core, uint32_t addr, uint32_t data, bool *write_done)
{
	struct cache *p_cache = p_core->p_cache;
	uint8_t idx = ADDR_IDX_GET(addr);

	if (cache_hit(p_cache, addr)) {
		*write_done = true;
		cache_write(p_cache, addr, data);

		// invalidate other cahces that hold now modified block
		if (cache_state_get(p_cache, idx) == MESI_SHARED) {
			bus_read_x(p_core, addr);
			// FIXME: might need to stall even that write_done!!
		}

		cache_state_set(p_cache, idx, MESI_MODIFIED);
	} else {
		*write_done = false;
		
		if (cache_state_get(p_cache, idx) == MESI_MODIFIED) {
			cache_evict_block(p_cache, idx);
			return;
		}

		// TODO: how this is delayed to next rr
		bus_read_x(p_core, addr);
	}
}

uint32_t core_read(struct core *p_core, uint32_t addr, bool *read_done)
{
	struct cache *p_cache = p_core->p_cache;
	uint8_t idx = ADDR_IDX_GET(addr);
	uint32_t data = 0;

	if (cache_hit(p_cache, addr)) {
		*read_done = true;
		data = cache_read(p_cache, addr);
	} else {
		*read_done = false;

		if (cache_state_get(p_cache, idx) == MESI_MODIFIED) {
			cache_evict_block(p_cache, idx);
			return 0;
		}

		// TODO: how this is delayed to next rr
		bus_read(p_core, addr);
	}

	return data;
}

void core_fetch_instruction(struct core *p_core)
{
	struct pipe *if_id = &p_core->pipe[IF_ID];
	uint32_t *imem = p_core->imem;
	uint32_t pc = p_core->pc.q;

	if (p_core->stall_decode || p_core->stall_mem) {
		dbg_verbose("[core%d][fetch] stall\n", p_core->idx);
		return;
	}

	if_id->ir.d = imem[pc & PC_MASK];
	if_id->npc.d = pc;

	dbg_verbose("[core%d][fetch] pc=%d\n", p_core->idx, pc);
}

void core_decode_instruction(struct core *p_core)
{
	struct pipe *if_id = &p_core->pipe[IF_ID];
	struct pipe *id_ex = &p_core->pipe[ID_EX];
	uint32_t sign_extention_mask = 0;
	uint32_t sign_extended_imm = 0;
	reg32_t *reg = p_core->reg;
	uint32_t inst = if_id->ir.q;
	uint16_t raw_info = 0;

	if (p_core->stall_mem) {
		dbg_verbose("[core%d][decode] stall\n", p_core->idx);
		return;
	}

	uint8_t op = INST_OP_GET(inst);
	uint8_t rd = INST_RD_GET(inst);
	uint8_t rs = INST_RS_GET(inst);
	uint8_t rt = INST_RT_GET(inst);
	uint16_t im = INST_IM_GET(inst);

	if (if_id->npc.q == INVALID_PC) {
		id_ex->npc.d = if_id->npc.q;

		if (!p_core->halt)
			p_core->pc.d = p_core->pc.q + 1;

		dbg_verbose("[core%d][decode]\n", p_core->idx);
		return;
	}

	if (core_hazard_raw(p_core, inst, &raw_info)) {
		dbg_verbose("[core%d][decode] RAW Hazard! pc=%d, inst=%08x, raw_info=%04x\n",
			    p_core->idx, if_id->npc.q, inst, raw_info);
		core_stall(p_core, ID_EX);
		return;
	} else {
		p_core->stall_decode = false;
	}

	// sign extend and store imm value in reg1 (D and Q)
	if (im & BIT(11)) {
		sign_extention_mask = ~(BIT(12) - 1);
	}

	p_core->reg[1].d = im | sign_extention_mask;
	p_core->reg[1].q = p_core->reg[1].d;

	dbg_verbose("[core%d][decode] pc=%d, inst=%08x, imm=%08x\n", p_core->idx,
		    if_id->npc.q, inst, p_core->reg[1].d);

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
	uint32_t sign_extention_mask = 0;

	if (p_core->stall_mem) {
		dbg_verbose("[core%d][execute] stall\n", p_core->idx);
		return;
	}

	if (id_ex->npc.q == INVALID_PC) {
		ex_mem->npc.d = id_ex->npc.q;
		ex_mem->dst.d = id_ex->dst.q;
		dbg_verbose("[core%d][execute]\n", p_core->idx);
		return;
	}

	if (IS_NEGATIVE(rsv)) {
		sign_extention_mask = ~(BIT(31 - rtv) - 1);
	}

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

	case OP_BEQ:
	case OP_BNE:
	case OP_BLT:
	case OP_BGT:
	case OP_BLE:
	case OP_BGE:
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
		dbg_warning("[core%d][execute] pc=%d, invalid op=%d\n", p_core->idx,
			    id_ex->npc.q, op);
		break;
	}

	dbg_verbose("[core%d][execute] pc=%d, op=%x, rsv=0x%x, rtv=0x%x, ,alu=0x%x\n",
		    p_core->idx, id_ex->npc.q, op, rsv, rtv, alu);

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
	uint32_t addr = ex_mem->alu.q;
	uint32_t data = 0;
	bool rw_done = false;
	bool rw_check = true;

	if (p_core->stall_mem) {
		if (!bus_user_in_queue(p_core->p_cache->p_bus, p_core->idx, NULL)) { // bus was cleared
			if (cache_hit(p_core->p_cache, addr)) {
				p_core->stall_mem = false;
			} else {
				dbg_verbose("[core%d][memory] miss2, mesi=%d, tag=%03x\n",
					    p_core->idx, cache_state_get(p_core->p_cache, ADDR_IDX_GET(addr)),
					    cache_tag_get(p_core->p_cache, ADDR_IDX_GET(addr)));
			}
		} else {
			// TODO: im in queue. what next? when will i get my turn?
			dbg_verbose("[core%d][memory] stall\n", p_core->idx);
			return;
		}
	}

	if (ex_mem->npc.q == INVALID_PC) {
		mem_wb->npc.d = ex_mem->npc.q;
		mem_wb->dst.d = ex_mem->dst.q;
		dbg_verbose("[core%d][memory]\n", p_core->idx);
		return;
	}

	if (op == OP_LW || op == OP_SW) {
		switch(op) {
		case OP_LW:
			data = core_read(p_core, addr, &rw_done);
			break;

		case OP_SW:
			data = ex_mem->rdv.q;
			core_write(p_core, addr, data, &rw_done);
			break;

		default:
			break;
		}

		dbg_verbose("[core%d][memory] pc=%d, %c, addr=%08x, data=%08x [%s]\n",
			    p_core->idx, ex_mem->npc.q, (op == OP_LW) ? 'R' : 'W', addr, data,
			    rw_done ? "hit" : "miss");

		if (rw_check && !rw_done) {
			core_stall(p_core, MEM_WB);
			return;
		}
	} else {
		dbg_verbose("[core%d][memory] pc=%d (no memory access)\n", p_core->idx, ex_mem->npc.q);
	}

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
	uint8_t op = INST_OP_GET(mem_wb->ir.q);
	uint32_t val = 0;
	uint8_t dst = 0;

	if (mem_wb->npc.q == INVALID_PC) {
		dbg_verbose("[core%d][writeback]\n", p_core->idx);
		return;
	}

	if (op < OP_BEQ || op == OP_JAL) {
		dst = mem_wb->dst.q & REG_MASK;
		val = mem_wb->alu.q;
	} else if (op == OP_LW) {
		dst = mem_wb->dst.q & REG_MASK;
		val = mem_wb->md.q;
	}

	reg[dst].d = val;

	dbg_verbose("[core%d][writeback] pc=%d, op=%d, dst=%d, val=%08x\n", p_core->idx, mem_wb->npc.q, op, dst, val);
}

void core_free(struct core *p_core)
{
	if (!p_core) {
		dbg_warning("invalid core\n");
		return;
	}

	if (p_core->p_cache) {
		cache_free(p_core->p_cache);
	}
	
	if (p_core->imem) {
		mem_free(p_core->imem);
	}
}

struct core *core_alloc(int idx)
{
	struct core *p_core = NULL;

	if (idx >= CORE_MAX) {
		dbg_error("invalid core_idx=%d, max cores allowed is %d\n", idx, CORE_MAX);
		return NULL;
	}

	p_core = calloc(1, sizeof(*p_core));
	if (!p_core) {
		print_error("Failed to allocate core");
		return NULL;
	}

	p_core->p_cache = cache_alloc();
	if (!p_core->p_cache) {
		dbg_error("Failed to allocate core cache\n");
		core_free(p_core);
		return NULL;
	}

	p_core->imem = mem_alloc(IMEM_LEN);
	if (!p_core->imem) {
		dbg_error("Failed to allocate core imem\n");
		core_free(p_core);
		return NULL;
	}

	p_core->idx = idx;

	return p_core;
}

int core_load(char **file_paths, struct core *p_core, uint32_t *mem, struct bus *p_bus)
{
	int res = 0;

	p_core->trace_path = file_paths[PATH_CORE0TRACE + p_core->idx];
	p_core->reg_dump_path = file_paths[PATH_REGOUT0 + p_core->idx];
	p_core->stats_dump_path = file_paths[PATH_STATS0 + p_core->idx];

	// TODO: redo this
	p_core->p_cache->p_bus = p_bus;
	p_core->p_cache->tsram_dump_path = file_paths[PATH_TSRAM0 + p_core->idx];
	p_core->p_cache->dsram_dump_path = file_paths[PATH_DSRAM0 + p_core->idx];
	p_core->p_cache->p_core = p_core;

	res = mem_load(file_paths[PATH_IMEME0 + p_core->idx], p_core->imem, IMEM_LEN, MEM_LOAD_FILE);
	if (res < 0) {
		return -1;
	}

	for (int i = 0; i < PIPE_MAX; i++) {
		p_core->pipe[i].npc.q = INVALID_PC;
		p_core->pipe[i].npc.d = INVALID_PC;
	}

	dbg_info("core%d loaded\n", p_core->idx);

	return 0;
}

void core_trace_pipe(FILE *fp, struct core *p_core)
{
	uint32_t pc;

	pc = p_core->pipe[IF_ID].npc.d;

	if (pc != INVALID_PC) {
		fprintf(fp, "%03d ", pc);
	} else {
		fprintf(fp, "--- ");
	}


	for (int i = ID_EX; i < PIPE_MAX + 1; i++) {
		pc = p_core->pipe[i - 1].npc.q;

		if (pc != INVALID_PC) {
			fprintf(fp, "%03d ", pc);
		} else {
			fprintf(fp, "--- ");
		}
	}
}

void core_trace_reg(FILE *fp, struct core *p_core)
{
	reg32_t *reg = p_core->reg;

	for (int i = 2; i < REG_MAX; i++) {
		fprintf(fp, "%08X ", reg[i].q);
	}
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

void core_stats_inc(struct core *p_core, uint8_t stat)
{
	p_core->stats[stat]++;
}

void core_snoop1_bus_rd_x(struct core *p_core)
{
	struct cache *p_cache = p_core->p_cache;
	struct bus *p_bus = p_cache->p_bus;
	uint8_t idx = ADDR_IDX_GET(p_bus->addr);

	if (p_bus->origid == p_core->idx) {
		return;
	}

	if (cache_hit(p_cache, p_bus->addr)) {
		if (cache_state_get(p_cache, idx) == MESI_MODIFIED) {
			cache_flush_block(p_cache, idx, true);
		}

		cache_state_set(p_cache, idx, MESI_INVALID);
	}
}

void core_snoop1_bus_rd(struct core *p_core)
{
	struct cache *p_cache = p_core->p_cache;
	struct bus *p_bus = p_cache->p_bus;
	uint8_t idx = ADDR_IDX_GET(p_bus->addr);

	if (p_bus->origid == p_core->idx) {
		return;
	}

	if (cache_hit(p_cache, p_bus->addr)) {
		if (cache_state_get(p_cache, idx) == MESI_MODIFIED) {
			cache_flush_block(p_cache, idx, true);
		}

		cache_state_set(p_cache, idx, MESI_SHARED);
	}
}

void core_snoop1_bus_flush(struct core *p_core)
{
	struct cache *p_cache = p_core->p_cache;
	struct bus *p_bus = p_cache->p_bus;
	uint8_t idx = ADDR_IDX_GET(p_bus->addr);

	if (bus_user_get(p_bus) == p_core->idx) {
		return;
	}

	if (p_bus->flusher == p_core->idx) {
		// flushing to other core
		cache_flush_block(p_cache, idx, true);
	} else {
		if (cache_hit(p_cache, p_bus->addr)) {
			p_bus->shared = true;
			switch (p_bus->rd_type) {
			case BUS_CMD_BUS_RD:
				if (cache_state_get(p_cache, idx) == MESI_EXCLUSIVE) {
					cache_state_set(p_cache, idx, MESI_SHARED);
					p_bus->shared = true; // FIXME: correct?
				}
				break;

			case BUS_CMD_BUS_RD_X:
				cache_state_set(p_cache, idx, MESI_INVALID);
				break;

			default:
				break;
			}
		}
	}
}

void core_snoop1(struct core *p_core)
{
	struct cache *p_cache = p_core->p_cache;
	struct bus *p_bus = p_cache->p_bus;

	/* first core to snoop rd/rd_x and has block 
	 * will place flush command on the bus */
	switch (bus_cmd_get(p_bus)) {
	case BUS_CMD_BUS_RD:
		core_snoop1_bus_rd(p_core);
		break;

	case BUS_CMD_BUS_RD_X:
		core_snoop1_bus_rd_x(p_core);
		break;

	case BUS_CMD_FLUSH:
		core_snoop1_bus_flush(p_core);
		break;

	default:
		break;
	}
}

void core_snoop2_bus_flush(struct core *p_core)
{
	struct cache *p_cache = p_core->p_cache;
	struct bus *p_bus = p_cache->p_bus;
	uint8_t idx = ADDR_IDX_GET(p_bus->addr);

	if (bus_user_get(p_bus) == p_core->idx) {
		if (p_bus->flusher == p_core->idx) {
			cache_flush_block(p_cache, idx, false);
		} else {
			cache_write(p_cache, p_bus->addr, p_bus->data);
			cache_state_set(p_cache, idx, p_bus->shared ? MESI_SHARED : MESI_EXCLUSIVE);
			cache_tag_set(p_cache, idx, ADDR_TAG_GET(p_bus->addr));

			dbg_verbose("[core%d][snoop] addr=%05x, data=%08x, from=%x, mesi=%d\n", p_core->idx,
				    p_bus->addr, p_bus->data, p_bus->origid, cache_state_get(p_cache, idx));

			if (p_bus->flush_cnt == BLOCK_LEN) {
				bus_trace(p_bus); // FIXME: must trace before clearing. not elegant
				bus_clear(p_bus);
			}
		}
	}
}

void core_snoop2(struct core *p_core)
{
	struct cache *p_cache = p_core->p_cache;
	struct bus *p_bus = p_cache->p_bus;

	if (p_bus->origid == p_core->idx &&
	    p_bus->flusher != p_core->idx) {
		return;
	}

	switch (bus_cmd_get(p_bus)) {
	case BUS_CMD_FLUSH:
		core_snoop2_bus_flush(p_core);
		break;

	default:
		break;
	}
}

void core_cycle(struct core *p_core)
{
	core_snoop2(p_core);
	core_fetch_instruction(p_core);
	core_decode_instruction(p_core);
	core_execute_instruction(p_core);
	core_memory_access(p_core);
	core_write_back(p_core);
	core_trace(p_core);
}

void core_clock_tick(struct core *p_core)
{
	reg32_t *reg = p_core->reg;
	struct pipe *pipe;

	for (int i = 0; i < REG_MAX; i++) {
		reg[i].q = reg[i].d;
	}
	
	for (int i = 0; i < PIPE_MAX; i++) {
		if (p_core->stall_decode && i == IF_ID) {
			continue;
		}

		if (p_core->stall_mem && i != MEM_WB) {
			continue;
		}

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
	if (p_core->done) {
		return true;
	}

	if (core_is_halt(p_core)) {
		for (int i = 0; i < PIPE_MAX; i++) {
			if (p_core->pipe[i].npc.q != INVALID_PC)
				return false;
		}

		dbg_info("[core%d] done. clk=%d\n", p_core->idx, g_clk);
		p_core->done = true;
	}

	return p_core->done;
}

int core_stats_dump(struct core *p_core)
{
	FILE *fp = NULL;

	if (!(fp = fopen(p_core->stats_dump_path, "w"))) {
		print_error("Failed to open \"%s\"", p_core->reg_dump_path);
		return -1;
	}

	for (int i = 0; i < STATS_MAX; i++) {
		fprintf(fp, "%s %d\n", core_stat_name_2_str[i], p_core->stats[i]);
	}

	fclose(fp);
	return 0;
}

int core_regs_dump(struct core *p_core)
{
	FILE *fp = NULL;

	if (!(fp = fopen(p_core->reg_dump_path, "w"))) {
		print_error("Failed to open \"%s\"", p_core->reg_dump_path);
		return -1;
	}

	for (int i = 2; i < REG_MAX; i++) {
		fprintf(fp, "%08x\n", p_core->reg[i].q);
	}

	fclose(fp);
	return 0;
}

int core_dump(struct core *p_core)
{
	int res = 0;

	res |= core_regs_dump(p_core);
	res |= core_stats_dump(p_core);

	return res;
}
