#ifndef _BUS_H_
#define _BUS_H_

#include <stdint.h>
#include <stdbool.h>

enum origid {
	ORIGID_CORE0,
	ORIGID_CORE1,
	ORIGID_CORE2,
	ORIGID_CORE3,
	ORIGID_MAIN_MEM,
	ORIGID_MAX
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

	// control
	bool rd_busy;
	uint8_t rd_user;
	uint8_t rd_type;
	uint8_t queue[4];
	uint8_t flusher;
	uint8_t flush_cnt;
	char *trace_path;
};

uint8_t bus_cmd_get(struct bus *p_bus);
void bus_read(struct core *p_core, uint32_t addr);
void bus_read_x(struct core *p_core, uint32_t addr);
int bus_trace(struct bus *p_bus);
void bus_init(struct bus *p_bus, char *path);
void bus_clear(struct bus *p_bus);
void bus_cmd_set(struct bus *p_bus, uint8_t orig_id, uint8_t cmd,
	uint32_t addr, uint32_t data);

#endif