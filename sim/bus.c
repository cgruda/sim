#include "dbg.h"
#include "bus.h"
#include <stdbool.h>
#include "cache.h"
#include "core.h"
#include "mem.h"
#include "common.h"

uint8_t bus_cmd_get(struct bus *p_bus)
{
	return (uint8_t)p_bus->cmd;
}

bool bus_rd_busy(struct bus *p_bus)
{
	return p_bus->rd_busy;
}

bool bus_available(struct bus *p_bus)
{
	return (bus_cmd_get(p_bus) == BUS_CMD_NONE);
}

void bus_cmd_set(struct bus *p_bus, uint8_t orig_id, uint8_t cmd,
	uint32_t addr, uint32_t data)
{
	p_bus->origid = orig_id;
	p_bus->cmd = cmd;
	p_bus->addr = addr;
	p_bus->data = data;
}

void bus_init(struct bus *p_bus, char *path)
{
	bus_cmd_set(p_bus, 0, 0, 0, 0);
	p_bus->shared = false;
	p_bus->rd_busy = false;
	p_bus->flush_cnt = 0;
	p_bus->trace_path = path;
	p_bus->rd_user = ORIGID_MAX;
	p_bus->flusher = ORIGID_MAX;
	p_bus->rd_type = BUS_CMD_NONE;
	// TODO: reset queue
}

void bus_read_cmd_set(struct bus *p_bus, uint8_t orig_id, uint32_t addr)
{
	if (bus_rd_busy(p_bus))
		dbg_warning("invalid BusRd! ongoing transaction of user %d\n", p_bus->rd_user);

	bus_cmd_set(p_bus, orig_id, BUS_CMD_BUS_RD, addr, 0);
	p_bus->shared = false; // FIXME: relevant?
	p_bus->rd_busy = true;
	p_bus->rd_user = orig_id;
	p_bus->rd_type = BUS_CMD_BUS_RD;
}

void bus_read(struct core *p_core, uint32_t addr)
{
	struct cache *p_cache = p_core->p_cache;
	struct bus *p_bus = p_cache->p_bus;

	if (!bus_rd_busy(p_bus)) {
		bus_read_cmd_set(p_bus, p_core->idx, addr);
		dbg_verbose("[bus][BusRD] orig=%x addr=%05x, rd_user=%x\n", p_bus->origid, p_bus->addr, p_bus->origid);
		// BIT_CLR(p_bus->rr_map, p_core->idx); // TODO: RR
	} else {
		// BIT_SET(p_bus->rr_map, p_core->idx);// TODO: RR
	}
}

void bus_read_x(struct core *p_core, uint32_t addr)
{
	struct cache *p_cache = p_core->p_cache;
	struct bus *p_bus = p_cache->p_bus;

	// TODO:
	dbg_error("no support\n");
}

int bus_trace(struct bus *p_bus)
{
	FILE *fp = NULL;

	if (!p_bus->cmd)
		return 0;

	if (!(fp = fopen(p_bus->trace_path, "a"))) {
		print_error("Failed to open \"%s\"", p_bus->trace_path);
		return -1;
	}

	fprintf(fp, "%d %x %x %05x %08x %x\n", g_clk, p_bus->origid,
		    p_bus->cmd, p_bus->addr, p_bus->data, p_bus->shared);
	
	fclose(fp);
	return 0;
}

void bus_clear(struct bus *p_bus)
{
	bus_cmd_set(p_bus, 0, 0, 0, 0);
	p_bus->shared = false;
	p_bus->rd_busy = false;
	p_bus->flush_cnt = 0;
	p_bus->rd_user = ORIGID_MAX;
	p_bus->rd_type = BUS_CMD_NONE;
	p_bus->flusher = ORIGID_MAX;
}
