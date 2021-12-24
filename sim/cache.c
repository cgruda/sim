#include <stdlib.h>
#include <stdbool.h>
#include "cache.h"
#include "bus.h"
#include "core.h"
#include "dbg.h"

struct cache *cache_alloc(void)
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

uint16_t cache_tag_get(struct cache *p_cache, uint8_t idx)
{
	struct tsram *p_tsram = &p_cache->tsram;

	return (uint16_t)p_tsram->block_info[idx].tag;
}

void cache_tag_set(struct cache *p_cache, uint8_t idx, uint16_t tag)
{
	struct tsram *p_tsram = &p_cache->tsram;

	p_tsram->block_info[idx].tag = tag;
}

bool cache_hit(struct cache *p_cache, uint32_t addr)
{
	uint8_t addr_idx = ADDR_IDX_GET(addr);
	uint16_t addr_tag = ADDR_TAG_GET(addr);

	uint16_t block_tag = cache_tag_get(p_cache, addr_idx);
	uint8_t block_state = cache_state_get(p_cache, addr_idx);
	
	return (block_tag == addr_tag) && (block_state != MESI_INVALID);
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
	errno_t err;

	err = fopen_s(&fp, p_cache->dsram_dump_path, "w");
	if (err || !fp) {
		print_error("Failed to open \"%s\"", p_cache->dsram_dump_path);
		return -1;
	}

	for (int i = 0; i < DSRAM_LEN; i++) {
		fprintf_s(fp, "%08X\n", p_cache->dsram.mem[i]);
	}

	fclose(fp);

	err = fopen_s(&fp, p_cache->tsram_dump_path, "w");
	if (err || !fp) {
		print_error("Failed to open \"%s\"", p_cache->tsram_dump_path);
		return -1;
	}

	for (int i = 0; i < BLOCK_CNT; i++) {
		fprintf_s(fp, "%08X\n", p_cache->tsram.mem[i]);
	}

	fclose(fp);
	return 0;
}

void cache_flush_block(struct cache *p_cache, uint8_t idx, bool shared)
{
	struct bus *p_bus = p_cache->p_bus;

	uint32_t tag = p_cache->tsram.block_info[idx].tag;
	uint32_t base_addr = (tag << ADDR_TAG_OFT) | (idx << ADDR_IDX_OFT);
	uint32_t flush_addr = base_addr + p_bus->flush_cnt;
	uint32_t flush_data = cache_read(p_cache, flush_addr);

	/* this fucntion is will be called 4 times during flush operation,
	 * each call in its clock cycle. addr to be flushed determined by
	 * flush counter */
	p_bus->flusher = p_cache->p_core->idx;
	bus_cmd_set(p_bus, p_cache->p_core->idx, BUS_CMD_FLUSH, flush_addr, flush_data);
	p_bus->shared = shared;
	p_bus->busy = true;
	p_bus->flush_cnt++;
}

void cache_evict_block(struct cache *p_cache, uint8_t idx)
{
	struct bus *p_bus = p_cache->p_bus;
	struct core *p_core = p_cache->p_core;

	/* does not matter if we win arbitration or not */
	if (!bus_user_in_queue(p_bus, p_core->idx, NULL)) {
		bus_user_queue_push(p_bus, p_core->idx);
	}

	/* we won arbitration */
	if (!bus_busy(p_bus)) {
		cache_flush_block(p_cache, idx, false);
		cache_state_set(p_cache, idx, MESI_INVALID);
		dbg_verbose("[cache%d][evict] idx=%x\n", p_core->idx, idx);
	}
}

void cache_bus_read(struct cache *p_cache, uint32_t addr)
{
	struct core *p_core = p_cache->p_core;
	struct bus *p_bus = p_cache->p_bus;

	/* does not matter if we win arbitration or not */
	if (!bus_user_in_queue(p_bus, p_core->idx, NULL)) {
		bus_user_queue_push(p_bus, p_core->idx);
	}

	/* we won arbitration */
	if (!bus_busy(p_bus)) {
		bus_read_cmd_set(p_bus, p_core->idx, addr);
		dbg_verbose("[bus][BusRd] orig=%x addr=%05x\n", p_bus->origid, p_bus->addr);
	}
}

void cache_bus_read_x(struct cache *p_cache, uint32_t addr)
{
	struct core *p_core = p_cache->p_core;
	struct bus *p_bus = p_cache->p_bus;

	/* does not matter if we win arbitration or not */
	if (!bus_user_in_queue(p_bus, p_core->idx, NULL)) {
		bus_user_queue_push(p_bus, p_core->idx);
	}

	/* we won arbitration */
	if (!bus_busy(p_bus)) {
		bus_read_x_cmd_set(p_bus, p_core->idx, addr);
		dbg_verbose("[bus][BusRdX] orig=%x addr=%05x\n", p_bus->origid, p_bus->addr);
	}
}
