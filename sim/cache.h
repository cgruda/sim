#ifndef _CACHE_H_
#define _CACHE_H_

#include <stdint.h>

#define BLOCK_LEN	4
#define BLOCK_CNT	64
#define DSRAM_LEN	((BLOCK_CNT) * (BLOCK_LEN))

enum mesi {
	INVALID,
	SHARED,
	EXCLUSIVE,
	MODIFIED,
};

struct block {
	uint32_t mem[BLOCK_LEN];
};

struct block_info {
	uint16_t tag  : 12,
		 mesi : 2;
};

struct dsram {
	union {
		struct block block[BLOCK_CNT];
		uint32_t mem[DSRAM_LEN];
	};
};

struct tsram {
	union {
		struct block_info block_info[BLOCK_CNT];
		uint16_t mem[BLOCK_CNT];
	};
};

struct cache {
	struct dsram dsram;
	struct tsram tsram;
};

#endif