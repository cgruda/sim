#ifndef _BUS_H_
#define _BUS_H_

#include <stdint.h>

enum bus_origid {
	ORIGID_CORE0,
	ORIGID_CORE1,
	ORIGID_CORE2,
	ORIGID_CORE3,
	ORIGID_MAIN_MEM,
	ORIGID_MAX
};

enum bus_cmd {
	CMD_NONE,
	CMD_BUSRD,
	CMD_BUSRDX,
	CMD_FLUSH,
	CMD_MAX,
};

struct bus {
	uint32_t bus_origid : 3,
		 bus_cmd    : 2,
		 bus_addr   : 20;
	uint32_t bus_data;
	uint32_t bus_shared : 1;
};

extern struct bus bus;

#endif