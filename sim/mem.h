#ifndef _MEM_H_
#define _MEM_H_

#include <stdint.h>

int mem_alloc(uint32_t **p_p_mem, int len);
int mem_load(char *path, uint32_t *mem, int len);
int mem_dump(char *path, uint32_t *mem, int len);

#endif