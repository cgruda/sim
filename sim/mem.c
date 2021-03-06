#include <stdlib.h>
#include "mem.h"
#include "bus.h"
#include "cache.h"
#include "dbg.h"

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

int mem_load(char *path, uint32_t *mem, int len, uint8_t load_mode, uint32_t *cnt)
{
	uint32_t *mem_start = mem;
	FILE *fp = NULL;
	errno_t err;

	/* for debug only */
	if (load_mode == MEM_LOAD_DUMMY) {
		for (int i = 0; i < len; i++) {
			mem[i] = i;
		}

		if (cnt) {
			*cnt = len;
		}

		return 0;
	}

	err = fopen_s(&fp, path, "r");
	if (err || !fp) {
		print_error("Failed to open \"%s\"", path);
		return -1;
	}

	while (!feof(fp)) {
		if (!fscanf_s(fp, "%x\n", mem++)) {
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

	if (cnt) {
		*cnt = mem - mem_start - 1;
	}

	fclose(fp);
	return 0;
}

int mem_dump(struct mem *p_mem)
{
	FILE *fp = NULL;
	errno_t err;

	err = fopen_s(&fp, p_mem->dump_path, "w");
	if (err || !fp) {
		print_error("Failed to open \"%s\"", p_mem->dump_path);
		return -1;
	}

	for (uint32_t i = 0; i <= p_mem->last_dump_addr; i++) {
		fprintf_s(fp, "%08X\n", p_mem->data[i]);
	}

	dbg_info("[mem] dump done\n");

	fclose(fp);
	return 0;
}

void mem_write(struct mem *p_mem, uint32_t addr, uint32_t data)
{
	/* so as to not print all memout */
	if ((addr & MEM_ADDR_MASK) > p_mem->last_dump_addr) {
		p_mem->last_dump_addr = addr & MEM_ADDR_MASK;
	}

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
	p_bus->flush_cnt++;

	dbg_verbose("[mem][flush] addr=%05x, data=%08x\n", flush_addr, flush_data);
}

void mem_snoop(struct mem *p_mem)
{
	struct bus *p_bus = p_mem->p_bus;

	if (p_bus->flusher == ORIGID_MAIN_MEM) {
		mem_flush_block(p_mem, p_bus->addr);
		return;
	}

	switch(bus_cmd_get(p_bus)) {
	/* core asked for data - if got here then no core has data (M) */
	case BUS_CMD_BUS_RD:
	case BUS_CMD_BUS_RD_X:
		dbg_verbose("[mem][snoop] orig=%x, cmd=%x, addr=%05x\n", p_bus->origid, p_bus->cmd, p_bus->addr);
		p_mem->flush_delay = MEM_DATA_DELAY - 2;
		p_bus->flusher = ORIGID_MAIN_MEM;
		p_bus->cmd = BUS_CMD_NONE;
		break;

	/* im flusing, or core flushing */
	case BUS_CMD_FLUSH:
		mem_write(p_mem, p_bus->addr, p_bus->data);

		if (bus_user_get(p_bus) == p_bus->origid) {
			/* core evicting a modified block */
			dbg_verbose("[mem][snoop][evict] orig=%x, addr=%05x, data=%08x, clear_bus=%d\n", p_bus->origid,
				p_bus->addr, p_bus->data, p_bus->flush_cnt == BLOCK_LEN);

			if (p_bus->flush_cnt == BLOCK_LEN) {
				bus_clear(p_bus);
				bus_user_queue_pop(p_bus);
			}
		}

		break;

	default:
		break;
	}
}
