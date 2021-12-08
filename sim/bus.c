#include "dbg.h"
#include "bus.h"
#include <stdbool.h>
#include "cache.h"
#include "core.h"
#include "mem.h"
#include "sim.h"

bool bus_user_in_queue(struct bus *p_bus, uint8_t user, uint8_t *p_pos)
{
	for (int i = 0; i < BUS_QUEUE_LEN; i++) {
		if (p_bus->user_queue[i] == user) {
			if (p_pos) {
				*p_pos = i;
			}

			return true;
		}
	}

	return false;
}

uint8_t bus_user_get(struct bus *p_bus)
{
	return p_bus->user_queue[0];
}

void bus_user_queue_push(struct bus *p_bus, uint8_t user)
{
	uint8_t pos;

	if (bus_user_in_queue(p_bus, user, &pos)) {
		dbg_warning("[bus] user %d already in queue (at pos %d)\n", user, pos);
		return;
	}

	for (int i = 0; i < BUS_QUEUE_LEN; i++) {
		if (p_bus->user_queue[i] == ORIGID_INVALID) {
			p_bus->user_queue[i] = user;
			break;
		}
	}
}

void bus_user_queue_pop(struct bus *p_bus)
{
	for (int i = 0; i < BUS_QUEUE_LEN - 1; i++) {
		p_bus->user_queue[i] = p_bus->user_queue[i + 1];
	}

	p_bus->user_queue[BUS_QUEUE_LEN - 1] = ORIGID_INVALID;
}

void bus_user_queue_reset(struct bus *p_bus)
{
	for (int i = 0; i < BUS_QUEUE_LEN; i++) {
		p_bus->user_queue[i] = ORIGID_INVALID;
	}
}

bool bus_user_queue_empty(struct bus *p_bus)
{
	for (int i = 0; i < BUS_QUEUE_LEN; i++) {
		if (p_bus->user_queue[i] != ORIGID_INVALID) {
			return false;
		}
	}

	return true;
}

uint8_t bus_cmd_get(struct bus *p_bus)
{
	return (uint8_t)p_bus->cmd;
}

bool bus_busy(struct bus *p_bus)
{
	return p_bus->busy;
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
	p_bus->busy = false;
	p_bus->flush_cnt = 0;
	p_bus->trace_path = path;
	p_bus->flusher = ORIGID_MAX;
	p_bus->rd_type = BUS_CMD_NONE;
	bus_user_queue_reset(p_bus);
}

void bus_read_cmd_set(struct bus *p_bus, uint8_t orig_id, uint32_t addr)
{
	if (bus_busy(p_bus)) {
		dbg_warning("invalid BusRd! ongoing transaction of user %d\n", bus_user_get(p_bus));
	}

	bus_cmd_set(p_bus, orig_id, BUS_CMD_BUS_RD, addr, 0);
	p_bus->shared = false; // FIXME: relevant?
	p_bus->busy = true;
	p_bus->rd_type = BUS_CMD_BUS_RD;
}

void bus_read_x_cmd_set(struct bus *p_bus, uint8_t orig_id, uint32_t addr)
{
	if (bus_busy(p_bus)) {
		dbg_warning("invalid BusRdX! ongoing transaction of user %d\n", bus_user_get(p_bus));
	}

	bus_cmd_set(p_bus, orig_id, BUS_CMD_BUS_RD_X, addr, 0);
	p_bus->shared = false;
	p_bus->busy = true;
	p_bus->rd_type = BUS_CMD_BUS_RD_X;
}

// TODO: move to cache module
void bus_read(struct core *p_core, uint32_t addr)
{
	struct cache *p_cache = p_core->p_cache;
	struct bus *p_bus = p_cache->p_bus;

	if (!bus_busy(p_bus)) {
		if (bus_user_queue_empty(p_bus)) {
			bus_user_queue_push(p_bus, p_core->idx);
		}

		bus_read_cmd_set(p_bus, p_core->idx, addr);
		dbg_verbose("[bus][BusRd] orig=%x addr=%05x\n", p_bus->origid, p_bus->addr);
	} else {
		if (!bus_user_in_queue(p_bus, p_core->idx, NULL)) {
			bus_user_queue_push(p_bus, p_core->idx);
		}
	}
}

void bus_read_x(struct core *p_core, uint32_t addr)
{
	struct cache *p_cache = p_core->p_cache;
	struct bus *p_bus = p_cache->p_bus;

	if (!bus_busy(p_bus)) {
		if (bus_user_queue_empty(p_bus)) {
			bus_user_queue_push(p_bus, p_core->idx);
		}

		bus_read_x_cmd_set(p_bus, p_core->idx, addr);
		dbg_verbose("[bus][BusRdX] orig=%x addr=%05x\n", p_bus->origid, p_bus->addr);
	} else {
		if (!bus_user_in_queue(p_bus, p_core->idx, NULL)) {
			bus_user_queue_push(p_bus, p_core->idx);
		}
	}
}

int bus_trace(struct bus *p_bus)
{
	FILE *fp = NULL;

	if (!p_bus->cmd) {
		return 0;
	}

	if (!(fp = fopen(p_bus->trace_path, "a"))) {
		print_error("Failed to open \"%s\"", p_bus->trace_path);
		return -1;
	}

	fprintf(fp, "%d %x %x %05x %08x %x\n", sim_clk, p_bus->origid,
		    p_bus->cmd, p_bus->addr, p_bus->data, p_bus->shared);
	
	fclose(fp);
	return 0;
}

void bus_clear(struct bus *p_bus)
{
	bus_cmd_set(p_bus, 0, 0, 0, 0);
	p_bus->shared = false;
	p_bus->busy = false;
	p_bus->flush_cnt = 0;
	p_bus->rd_type = BUS_CMD_NONE;
	p_bus->flusher = ORIGID_MAX;
	bus_user_queue_pop(p_bus);
}
