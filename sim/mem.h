#ifndef _MEM_H_
#define _MEM_H_

#include <stdint.h>

#define MEM_LEN (1 << 20)
#define MEM_ADDR_MASK 0x000FFFFF
#define MEM_DATA_DELAY 16

enum mem_load_mode {
	MEM_LOAD_FILE,
	MEM_LOAD_DUMMY,
};

struct bus;

struct mem {
	uint32_t *data;
	struct bus *p_bus;
	uint8_t flush_delay;
	char *dump_path;
};

uint32_t *mem_alloc(int len);
void mem_free(uint32_t *p_mem);
int mem_load(char *path, uint32_t *mem, int len, uint8_t load_mode);
int mem_dump(struct mem *p_mem);
void mem_write(struct mem *p_mem, uint32_t addr, uint32_t data);
uint32_t mem_read(struct mem *p_mem, uint32_t addr);
void mem_snoop(struct mem *p_mem);

#endif
