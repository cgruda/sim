#include "dbg.h"
#include <stdlib.h>
#include "cache.h"

struct cache *cache_init()
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