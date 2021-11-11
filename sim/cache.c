#include "dbg.h"
#include <stdlib.h>
#include "cache.h"

struct cache *cache_init()
{
	struct cache *p_cache = calloc(1, sizeof(*p_cache));
	if (!p_cache) {
		dbg_error("failed to allocate cache\n");
		print_error();
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

uint32_t cache_read(uint32_t addr)
{

}

uint32_t cache_write(uint32_t addr)
{

}