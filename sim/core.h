#ifndef _CORE_H_
#define _CORE_H_

#include <stdint.h>
#include <stdbool.h>
#include "cache.h"

#define REG_MAX 16
#define IMEM_LEN 1024

struct cache;

enum core_id {
	CORE_0,
	CORE_1,
	CORE_2,
	CORE_3,
	CORE_MAX
};

enum op {
	OP_ADD,
	OP_SUB,
	OP_AND,
	OP_OR,
	OP_XOR,
	OP_MUL,
	OP_SLL,
	OP_SRA,
	OP_SRL,
	OP_BEQ,
	OP_BNE,
	OP_BLT,
	OP_BGT,
	OP_BLE,
	OP_BGE,
	OP_JAL,
	OP_LW,
	OP_SW,
	OP_HALT,
	OP_MAX,
};

enum pipe_stage {
	IF_ID,
	ID_EX,
	EX_MEM,
	MEM_WB,
	PIPE_MAX
};

enum stats {
	STATS_CYCLE,
	STATS_INSTRUCTION,
	STATS_READ_HIT,
	STATS_WRITE_HIT,
	STATS_READ_MISS,
	STATS_WRITE_MISS,
	STATS_DECODE_STALL,
	STATS_MEM_STALL,
	STATS_MAX,
};

typedef struct reg32 {
	uint32_t d;
	uint32_t q;
} reg32_t;

struct pipe {
	reg32_t npc;
	reg32_t rsv;
	reg32_t rtv;
	reg32_t rdv;
	reg32_t alu;
	reg32_t dst;
	reg32_t md;
	reg32_t ir;
};

struct core {
	int idx;
	reg32_t pc;
	reg32_t reg[REG_MAX];
	uint32_t *imem;
	struct cache *cache;
	struct pipe pipe[PIPE_MAX];
	bool halt;
	uint32_t *mem;
	char *trace_path;
	char *stats_path;
	int stats[STATS_MAX];
};

inline bool core_is_halt(struct core *p_core)
{
	return p_core->halt;
}

struct core *core_init(int idx);
int core_load(char **file_paths, struct core *p_core, uint32_t *main_mem);
void core_free(struct core *p_core);
void core_cycle(struct core *p_core);
void core_clock_tick(struct core *p_core);

#endif