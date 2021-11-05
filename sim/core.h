#ifndef _CORE_H_
#define _CORE_H_

#include <stdint.h>
#include <stdbool.h>

#define REG_MAX 16

enum opcode {
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
	OP_MAX
};

struct cache;

struct inst {
	union {
		uint32_t op : 8,
			 rd : 4,
			 rs : 4,
			 rt : 4,
			 im : 12;
		uint32_t word;
	};
};

struct core {
	uint32_t pc : 10;
	uint32_t regs[REG_MAX];
	struct cache cache;
};

#endif