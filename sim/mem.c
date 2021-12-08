#include "dbg.h"
#include <stdlib.h>
#include "mem.h"
#include "bus.h"
#include "cache.h"
#include "common.h"

uint32_t *mem_alloc(int len)
{
	uint32_t *p_mem = calloc(len, sizeof(uint32_t));
	if (!p_mem) {
		print_error("Failed to allocate memory");
		return NULL;
	}

	return p_mem;
}

void mem_free(uint32_t *p_mem)
{
	if (!p_mem) {
		dbg_warning("invalid memory\n");
		return;
	}

	free(p_mem);
}

int mem_load(char *path, uint32_t *mem, int len, uint8_t load_mode)
{
	uint32_t *mem_start = mem;
	FILE *fp = NULL;

	if (load_mode == MEM_LOAD_DUMMY) {
		for (int i = 0; i < len; i++) {
			mem[i] = i;
		}

		return 0;
	}

	if (!(fp = fopen(path, "r"))) {
		print_error("Failed to open \"%s\"", path);
		return -1;
	}

	while (!feof(fp)) {
		if (!fscanf(fp, "%x\n", mem++)) {
			dbg_error("\"%s\" scan error\n", path);
			fclose(fp);
			return -1;
		}
		if (mem - mem_start > len) {
			dbg_error("\"%s\" exceeds %d bytes\n", path, len);
			fclose(fp);
			return -1;
		}
	}

	fclose(fp);
	return 0;
}

int mem_dump(struct mem *p_mem)
{
	FILE *fp = NULL;

	if (!(fp = fopen(p_mem->dump_path, "w"))) {
		print_error("Failed to open \"%s\"", p_mem->dump_path);
		return -1;
	}

	for (int i = 0; i < MEM_LEN; i++) {
		fprintf(fp, "%08x\n", p_mem->data[i]);
	}

	dbg_info("[mem] dump done\n");

	fclose(fp);
	return 0;
}

void mem_write(struct mem *p_mem, uint32_t addr, uint32_t data)
{
	p_mem->data[addr & MEM_ADDR_MASK] = data;
}

uint32_t mem_read(struct mem *p_mem, uint32_t addr)
{
	return p_mem->data[addr & MEM_ADDR_MASK];
}

void mem_flush_block(struct mem *p_mem, uint32_t addr)
{
	struct bus *p_bus = p_mem->p_bus;
	p_bus->flusher = ORIGID_MAIN_MEM;
	uint32_t base_addr = addr & ~ADDR_OFT_MSK;

	if (p_mem->flush_delay) {
		p_mem->flush_delay--;
		return;
	}

	uint32_t flush_addr = base_addr + p_bus->flush_cnt;
	uint32_t flush_data = mem_read(p_mem, flush_addr);

	bus_cmd_set(p_bus, ORIGID_MAIN_MEM, BUS_CMD_FLUSH, flush_addr, flush_data);
	dbg_verbose("[mem][flush] addr=%05x, data=%08x\n", flush_addr, flush_data);
	// FIXME:NOTE: i dont set bus_shared!!
	p_bus->flush_cnt++;
}

void mem_snoop(struct mem *p_mem)
{
	struct bus *p_bus = p_mem->p_bus;

	if (p_bus->flusher == ORIGID_MAIN_MEM) {
		mem_flush_block(p_mem, p_bus->addr);
		return;
	}

	switch(bus_cmd_get(p_bus)) {
	case BUS_CMD_BUS_RD:
	case BUS_CMD_BUS_RD_X:
		dbg_verbose("[mem][snoop] orig=%x, cmd=%x, addr=%05x\n", p_bus->origid, p_bus->cmd, p_bus->addr);
		p_mem->flush_delay = MEM_DATA_DELAY - 2;
		p_bus->flusher = ORIGID_MAIN_MEM;
		p_bus->cmd = BUS_CMD_NONE;
		break;

	case BUS_CMD_FLUSH:
		mem_write(p_mem, p_bus->addr, p_bus->data);

		if (bus_user_get(p_bus) == p_bus->origid) {
			// core evicting a modified block
			dbg_verbose("[mem][snoop][evict] orig=%x, addr=%05x, data=%08x\n", p_bus->origid,
				p_bus->addr, p_bus->data);

			if (p_bus->flush_cnt == BLOCK_LEN) {
				bus_clear(p_bus);
			}
		}

		break;

	default:
		break;
	}
}
