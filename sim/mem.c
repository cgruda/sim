#include "dbg.h"
#include <stdlib.h>
#include "mem.h"

uint32_t *mem_init(int len)
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
		dbg_warning("Invalid memory\n");
		return;
	}

	free(p_mem);
}

int mem_load(char *path, uint32_t *mem, int len)
{
	uint32_t *mem_start = mem;
	FILE *fp = NULL;

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

int mem_dump(char *path, uint32_t *mem, int len)
{
	FILE *fp = NULL;

	if (!(fp = fopen(path, "w"))) {
		print_error("Failed to open \"%s\"", path);
		return -1;
	}

	for (int i = 0; i < len; i++)
		fprintf(fp, "%08x\n", mem[i]);

	fclose(fp);
	return 0;
}

void mem_write(uint32_t *mem, uint32_t addr, uint32_t data)
{
	mem[addr] = data;
}

uint32_t mem_read(uint32_t *mem, uint32_t addr)
{
	return mem[addr];
}