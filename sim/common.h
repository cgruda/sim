#ifndef _COMMON_H_
#define _COMMON_H_

enum files {
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

#define BIT(x)	(1U << (x))

extern int g_clk;
const char *default_sim_files_paths[PATH_MAX];


#endif