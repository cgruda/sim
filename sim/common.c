#include "common.h"

const char *default_paths[PATH_MAX] = {
	[PATH_IMEME0]     = "imem0.txt",
	[PATH_IMEME1]     = "imem1.txt",
	[PATH_IMEME2]     = "imem2.txt",
	[PATH_IMEME3]     = "imem3.txt",
	[PATH_MEMIN]      = "memin.txt",
	[PATH_MEMOUT]     = "memout.txt",
	[PATH_REGOUT0]    = "regout0.txt",
	[PATH_REGOUT1]    = "regout1.txt",
	[PATH_REGOUT2]    = "regout2.txt",
	[PATH_REGOUT3]    = "regout3.txt",
	[PATH_CORE0TRACE] = "core0trace.txt",
	[PATH_CORE1TRACE] = "core1trace.txt",
	[PATH_CORE2TRACE] = "core2trace.txt",
	[PATH_CORE3TRACE] = "core3trace.txt",
	[PATH_BUSTRACE]   = "bustrace.txt",
	[PATH_DSRAM0]     = "dsram0.txt",
	[PATH_DSRAM1]     = "dsram1.txt",
	[PATH_DSRAM2]     = "dsram2.txt",
	[PATH_DSRAM3]     = "dsram3.txt",
	[PATH_TSRAM0]     = "tsram0.txt",
	[PATH_TSRAM1]     = "tsram1.txt",
	[PATH_TSRAM2]     = "tsram2.txt",
	[PATH_TSRAM3]     = "tsram3.txt",
	[PATH_STATS0]     = "stats0.txt",
	[PATH_STATS1]     = "stats1.txt",
	[PATH_STATS2]     = "stats2.txt",
	[PATH_STATS3]     = "stats3.txt",
};

int g_clk = 0;
