#ifndef _CORE_H_
#define _CORE_H_

#include <stdint.h>
#include <stdbool.h>
#include "cache.h"

#define REG_MAX 16
#define IMEM_LEN 1024

struct cache;

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
	reg32_t imm;
	reg32_t md;
	reg32_t ir;
	reg32_t rw;
};

struct core {
	reg32_t pc;
	reg32_t reg[REG_MAX];
	uint32_t *imem;
	struct cache *cache;
	struct pipe pipe[PIPE_MAX];
	bool halt;
	uint32_t *mem;
};

#endif