#include "dbg.h"
#include <stdlib.h>
#include "mem.h"

int mem_alloc(uint32_t **p_p_mem, int len)
{
	*p_p_mem = calloc(len, sizeof(uint32_t));
	if (!*p_p_mem) {
		dbg_error("failed to allocate memory\n");
		print_error();
		return -1;
	}

	return 0;
}

int mem_free(uint32_t **p_p_mem, int len)
{
	if (p_p_mem && *p_p_mem) {
		free(*p_p_mem);
		*p_p_mem  = NULL;
	}

	return 0;
}

int mem_load(char *path, uint32_t *mem, int len)
{
	uint32_t *mem_start = mem;
	FILE *fp = NULL;

	if (!(fp = fopen(path, "r"))) {
		dbg_error("failed to open \"%s\"\n", path);
		print_error();
		return -1;
	}

	while(!feof(fp)) {
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
		dbg_error("failed to open \"%s\"\n", path);
		print_error();
		return -1;
	}

	for (int i = 0; i < len; i++)
		fprintf(fp, "%08x\n", mem[i]);

	fclose(fp);
	return 0;
}