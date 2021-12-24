#ifndef _BUS_H_
#define _BUS_H_

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#define BUS_QUEUE_LEN	4

enum origid {
	ORIGID_CORE0,
	ORIGID_CORE1,
	ORIGID_CORE2,
	ORIGID_CORE3,
	ORIGID_MAIN_MEM,
	ORIGID_INVALID,
	ORIGID_MAX = ORIGID_INVALID
};

enum bus_cmd {
	BUS_CMD_NONE,
	BUS_CMD_BUS_RD,
	BUS_CMD_BUS_RD_X,
	BUS_CMD_FLUSH,
	BUS_CMD_MAX,
};

struct bus {
	uint32_t origid : 3,
		 cmd    : 2,
		 addr   : 20;
	uint32_t data;
	uint32_t shared : 1;

	bool busy;
	uint8_t user_queue[BUS_QUEUE_LEN];
	uint8_t flusher;
	uint8_t flush_cnt;
	FILE *trace_fp;
};

bool bus_user_in_queue(struct bus *p_bus, uint8_t user, uint8_t *p_pos);
void bus_user_queue_pop(struct bus *p_bus);
uint8_t bus_user_get(struct bus *p_bus);
void bus_user_set(struct bus *p_bus, uint8_t user);
uint8_t bus_cmd_get(struct bus *p_bus);
void bus_trace(struct bus *p_bus);
int bus_init(struct bus *p_bus, char *path);
void bus_clear(struct bus *p_bus);
bool bus_busy(struct bus *p_bus);
void bus_user_queue_push(struct bus *p_bus, uint8_t user);
void bus_read_cmd_set(struct bus *p_bus, uint8_t orig_id, uint32_t addr);
void bus_read_x_cmd_set(struct bus *p_bus, uint8_t orig_id, uint32_t addr);
void bus_cmd_set(struct bus *p_bus, uint8_t orig_id, uint8_t cmd,
	uint32_t addr, uint32_t data);

#endif
