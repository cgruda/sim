#ifndef _CACHE_H_
#define _CACHE_H_

#include <stdint.h>

#define BLOCK_LEN	4
#define BLOCK_CNT	64
#define DSRAM_LEN	((BLOCK_CNT) * (BLOCK_LEN))

#define ADDR_TAG_OFT	8
#define ADDR_TAG_MSK	0x000FFF00
#define ADDR_IDX_OFT	2
#define ADDR_IDX_MSK	0x000000FC
#define ADDR_OFT_OFT	0
#define ADDR_OFT_MSK	0x00000003

#define ADDR_TAG_GET(addr)	(((addr) & (ADDR_TAG_MSK)) >> (ADDR_TAG_OFT))
#define ADDR_IDX_GET(addr)	(((addr) & (ADDR_IDX_MSK)) >> (ADDR_IDX_OFT))
#define ADDR_OFT_GET(addr)	(((addr) & (ADDR_OFT_MSK)) >> (ADDR_OFT_OFT))

enum mesi {
	MESI_INVALID,
	MESI_SHARED,
	MESI_EXCLUSIVE,
	MESI_MODIFIED,
	MESI_MAX
};

struct core;
struct bus;

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
	struct bus *p_bus;
	struct core *p_core;

	char *dsram_dump_path;
	char *tsram_dump_path;
};

struct cache *cache_alloc(void);
void cache_free(struct cache *p_cache);
bool cache_hit(struct cache *p_cache, uint32_t addr);
void cache_write(struct cache *p_cache, uint32_t addr, uint32_t data);
uint32_t cache_read(struct cache *p_cache, uint32_t addr);
void cache_block_state_set(struct cache *p_cache, uint32_t addr, uint8_t state);
void cache_state_set(struct cache *p_cache, uint8_t idx, uint8_t state);
uint8_t cache_state_get(struct cache *p_cache, uint8_t idx);
bool cache_last_addr_in_block(uint32_t addr);
int cache_dump(struct cache *p_cache);
void cache_flush_block(struct cache *p_cache, uint8_t idx, bool shared);
void cache_evict_block(struct cache *p_cache, uint8_t idx);
uint16_t cache_tag_get(struct cache *p_cache, uint8_t idx);
void cache_tag_set(struct cache *p_cache, uint8_t idx, uint16_t tag);

#endif