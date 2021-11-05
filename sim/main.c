#include <stdio.h>
#include "dbg.h"

#define CORE_CNT	4
#define ARGC_CNT	27

enum files {
	FP_IMEME0,
	FP_IMEME1,
	FP_IMEME2,
	FP_IMEME3,
	FP_MEMIN,
	FP_MEMOUT,
	FP_REGOUT0,
	FP_REGOUT1,
	FP_REGOUT2,
	FP_REGOUT3,
	FP_CORE0TRACE,
	FP_CORE1TRACE,
	FP_CORE2TRACE,
	FP_CORE3TRACE,
	FP_BUSTRACE,
	FP_DSRAM0,
	FP_DSRAM1,
	FP_DSRAM2,
	FP_DSRAM3,
	FP_TSRAM0,
	FP_TSRAM1,
	FP_TSRAM2,
	FP_TSRAM3,
	FP_STATS0,
	FP_STATS1,
	FP_STATS2,
	FP_STATS3,
	FP_MAX
};

const char *default_sim_files_pathes[FP_MAX] = {
	[FP_IMEME0]     = "imem0.txt",
	[FP_IMEME1]     = "imem1.txt",
	[FP_IMEME2]     = "imem2.txt",
	[FP_IMEME3]     = "imem3.txt",
	[FP_MEMIN]      = "memin.txt",
	[FP_MEMOUT]     = "memout.txt",
	[FP_REGOUT0]    = "regout0.txt",
	[FP_REGOUT1]    = "regout1.txt",
	[FP_REGOUT2]    = "regout2.txt",
	[FP_REGOUT3]    = "regout3.txt",
	[FP_CORE0TRACE] = "core0trace.txt",
	[FP_CORE1TRACE] = "core1trace.txt",
	[FP_CORE2TRACE] = "core2trace.txt",
	[FP_CORE3TRACE] = "core3trace.txt",
	[FP_BUSTRACE]   = "bustrace.txt",
	[FP_DSRAM0]     = "dsram0.txt",
	[FP_DSRAM1]     = "dsram1.txt",
	[FP_DSRAM2]     = "dsram2.txt",
	[FP_DSRAM3]     = "dsram3.txt",
	[FP_TSRAM0]     = "tsram0.txt",
	[FP_TSRAM1]     = "tsram1.txt",
	[FP_TSRAM2]     = "tsram2.txt",
	[FP_TSRAM3]     = "tsram3.txt",
	[FP_STATS0]     = "stats0.txt",
	[FP_STATS1]     = "stats1.txt",
	[FP_STATS2]     = "stats2.txt",
	[FP_STATS3]     = "stats3.txt",
};

struct sim_env {
	FILE *fp[FP_MAX];
};

void sim_init(struct sim_env *p_env, int argc, char **argv)
{
	char **sim_files_paths = NULL;

	if (!argc) {
		sim_files_paths = &default_sim_files_pathes;
	} else if (argc == ARGC_CNT) {
		sim_files_paths = argv;
	} else {
		dbg_error("invalid input (received %d args, expected %d)\n", argc, ARGC_CNT);
		return -1;
	}

	for (int i = 0; i < FP_MAX; i++) {
		char *mode = (i < FP_MEMOUT) ? "r" : "w";
		p_env->fp[i] = fopen(sim_files_paths, mode);
		if (!p_env->fp[i]) {
			print_error();
			return -1; // FIXME: need to close files that did open
		}
	}
}

int main(int argc, char **argv)
{
	struct sim_env env = {0};

	sim_init(&env, --argc, ++argv);

	dbg_verbose("hello sim!\n");

	return 0;
}