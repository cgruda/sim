#include "dbg.h"
#include <stdlib.h>
#include <stdbool.h>
#include "cache.h"
#include "bus.h"

#define ADDR_TAG_OFT	8
#define ADDR_TAG_MSK	0x000FFF00
#define ADDR_IDX_OFT	2
#define ADDR_IDX_MSK	0x00F000FC
#define ADDR_OFT_OFT	0
#define ADDR_OFT_MSK	0x00000003

#define ADDR_TAG_GET(addr)	(((addr) & (ADDR_TAG_MSK)) >> (ADDR_TAG_OFT))
#define ADDR_IDX_GET(addr)	(((addr) & (ADDR_IDX_MSK)) >> (ADDR_IDX_OFT))
#define ADDR_OFT_GET(addr)	(((addr) & (ADDR_OFT_MSK)) >> (ADDR_OFT_OFT))

struct cache *cache_alloc()
{
	struct cache *p_cache = calloc(1, sizeof(*p_cache));
	if (!p_cache) {
		print_error("Failed to allocate cache");
		return NULL;
	}

	return p_cache;
}

void cache_free(struct cache *p_cache)
{
	if (!p_cache) {
		dbg_warning("Invalid cache\n");
		return;
	}

	free(p_cache);
}

uint8_t cache_block_state_get(struct cache *p_cache, int idx)
{
	struct tsram *p_tsram = &p_cache->tsram;

	return (uint8_t)p_tsram->block_info[idx].mesi;
}

void cache_block_state_set(struct cache *p_cache, int idx, uint8_t state)
{
	struct tsram *p_tsram = &p_cache->tsram;

	if (state >= MESI_MAX)
		dbg_warning("Invalid MESI state=%d\n", state);

	p_tsram->block_info[idx].mesi = state;
}

bool is_addr_in_cache(struct cache *p_cache, uint32_t addr)
{
	struct tsram *p_tsram = &p_cache->tsram;
	uint8_t idx = ADDR_IDX_GET(addr);
	uint16_t tag = ADDR_TAG_GET(addr);
	
	return p_tsram->block_info[idx].tag == tag;
}

void cache_write(struct cache *p_cache, uint32_t addr, uint32_t data)
{
	struct dsram *p_dsram = &p_cache->dsram;
	uint32_t idx = ADDR_IDX_GET(addr);
	uint32_t oft = ADDR_OFT_GET(addr);

	if (!is_addr_in_cache(p_cache, addr))
		dbg_warning("Invalid cache access\n");

	p_dsram->block[idx].mem[oft];
}

uint32_t cache_read(struct cache *p_cache, uint32_t addr)
{
	struct dsram *p_dsram = &p_cache->dsram;
	uint32_t idx = ADDR_IDX_GET(addr);
	uint32_t oft = ADDR_OFT_GET(addr);

	if (!is_addr_in_cache(p_cache, addr))
		dbg_warning("Invalid cache access\n");

	return p_dsram->block[idx].mem[oft];
}

