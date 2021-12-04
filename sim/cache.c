#include "dbg.h"
#include <stdlib.h>
#include <stdbool.h>
#include "cache.h"
#include "bus.h"
#include "core.h"

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
		dbg_warning("invalid cache\n");
		return;
	}

	free(p_cache);
}

uint8_t cache_state_get(struct cache *p_cache, uint8_t idx)
{
	struct tsram *p_tsram = &p_cache->tsram;

	return (uint8_t)p_tsram->block_info[idx].mesi;
}

void cache_state_set(struct cache *p_cache, uint8_t idx, uint8_t state)
{
	struct tsram *p_tsram = &p_cache->tsram;

	p_tsram->block_info[idx].mesi = state;
}

bool cache_hit(struct cache *p_cache, uint32_t addr)
{
	struct tsram *p_tsram = &p_cache->tsram;

	uint8_t addr_idx = ADDR_IDX_GET(addr);
	uint16_t addr_tag = ADDR_TAG_GET(addr);

	uint16_t block_tag = p_tsram->block_info[addr_idx].tag;
	uint8_t block_state = cache_state_get(p_cache, addr_idx);
	
	return (block_tag == addr_tag) && (block_state != MESI_INVALID);
}

bool cache_last_addr_in_block(uint32_t addr)
{
	return (ADDR_OFT_GET(addr) == ADDR_OFT_MSK);
}

void cache_write(struct cache *p_cache, uint32_t addr, uint32_t data)
{
	struct dsram *p_dsram = &p_cache->dsram;
	uint32_t idx = ADDR_IDX_GET(addr);
	uint32_t oft = ADDR_OFT_GET(addr);

	p_dsram->block[idx].mem[oft] = data;
}

uint32_t cache_read(struct cache *p_cache, uint32_t addr)
{
	struct dsram *p_dsram = &p_cache->dsram;
	uint32_t idx = ADDR_IDX_GET(addr);
	uint32_t oft = ADDR_OFT_GET(addr);

	return p_dsram->block[idx].mem[oft];
}

int cache_dump(struct cache *p_cache)
{
	FILE *fp = NULL;

	if (!(fp = fopen(p_cache->dsram_dump_path, "w"))) {
		print_error("Failed to open \"%s\"", p_cache->dsram_dump_path);
		return -1;
	}

	for (int i = 0; i < DSRAM_LEN; i++)
		fprintf(fp, "%08x\n", p_cache->dsram.mem[i]);

	fclose(fp);

	if (!(fp = fopen(p_cache->tsram_dump_path, "w"))) {
		print_error("Failed to open \"%s\"", p_cache->tsram_dump_path);
		return -1;
	}

	for (int i = 0; i < BLOCK_CNT; i++)
		fprintf(fp, "%08x\n", p_cache->tsram.mem[i]);

	fclose(fp);
	return 0;
}

void cache_flush_block(struct cache *p_cache, uint8_t idx)
{
	struct bus *p_bus = p_cache->p_bus;

	uint32_t tag = p_cache->tsram.block_info[idx].tag;
	uint32_t base_addr = (tag << ADDR_TAG_OFT) | (idx << ADDR_IDX_OFT);
	uint32_t flush_addr = base_addr + p_bus->flush_cnt;
	uint32_t flush_data = cache_read(p_cache, flush_addr);

	p_bus->flusher = p_cache->p_core->idx;
	bus_cmd_set(p_bus, p_cache->p_core->idx, BUS_CMD_FLUSH, flush_addr, flush_data);
	p_bus->shared = true;
	p_bus->flush_cnt++;
}
