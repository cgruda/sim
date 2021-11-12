#ifndef _MEM_H_
#define _MEM_H_

#include <stdint.h>

#define MEM_LEN (1 << 20)

uint32_t *mem_init(int len);
void mem_free(uint32_t *p_mem);
int mem_load(char *path, uint32_t *mem, int len);
int mem_dump(char *path, uint32_t *mem, int len);
void mem_write(uint32_t *mem, uint32_t addr, uint32_t data);
uint32_t mem_read(uint32_t *mem, uint32_t addr);

#endif