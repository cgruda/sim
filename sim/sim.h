#ifndef _SIM_H_
#define _SIM_H_

#include "core.h"
#include "bus.h"
#include "mem.h"

enum sim_paths {
	PATH_IMEME0,
	PATH_IMEME1,
	PATH_IMEME2,
	PATH_IMEME3,
	PATH_MEMIN,
	PATH_MEMOUT,
	PATH_REGOUT0,
	PATH_REGOUT1,
	PATH_REGOUT2,
	PATH_REGOUT3,
	PATH_CORE0TRACE,
	PATH_CORE1TRACE,
	PATH_CORE2TRACE,
	PATH_CORE3TRACE,
	PATH_BUSTRACE,
	PATH_DSRAM0,
	PATH_DSRAM1,
	PATH_DSRAM2,
	PATH_DSRAM3,
	PATH_TSRAM0,
	PATH_TSRAM1,
	PATH_TSRAM2,
	PATH_TSRAM3,
	PATH_STATS0,
	PATH_STATS1,
	PATH_STATS2,
	PATH_STATS3,
	PATH_MAX
};

struct sim_env {
	char **paths;

	struct core core[CORE_MAX];
	struct bus bus;
	struct mem mem;

	uint8_t core_done_bitmap;
	bool run;
};

extern int sim_clk;

int sim_init(struct sim_env *p_env, int argc, char **argv);
void sim_run(struct sim_env *p_env);
void sim_dump(struct sim_env *p_env);
int sim_cleanup(struct sim_env *p_env);

#endif
