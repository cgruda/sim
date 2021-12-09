#include "dbg.h"
#include <stdio.h>
#include <stdint.h>
#include "sim.h"
#include "core.h"
#include "mem.h"
#include "bus.h"
#include "cache.h"

#define ARGC_CNT	27
#define MAX_ITERATIONS  1000 // FIXME: for debug
#define MAIN_MEM_MODE	MEM_LOAD_DUMMY
// #define MAIN_MEM_MODE	MEM_LOAD_FILE

int sim_clk = 0;

const char *sim_default_paths[PATH_MAX] = {
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

int sim_cleanup(struct sim_env *p_env)
{
	if (p_env->mem.data) {
		mem_free(p_env->mem.data);
	}

	for (int i = 0; i < CORE_MAX; i++) {
		core_free(&p_env->core[i]);
	}

	dbg_info("simulation cleanup done\n");

	return 0;
}

void sim_remove_old_output_files(char **file_paths)
{
	for (int i = PATH_MEMOUT; i < PATH_MAX; i++) {
		remove(file_paths[i]);
	}
}

// TODO: rewrite
int sim_init(struct sim_env *p_env, int argc, char **argv)
{
	int res = 0;
	p_env->run = true;

	if (!argc) {
		p_env->paths = (char **)&sim_default_paths;
	} else if (argc == ARGC_CNT) {
		p_env->paths = argv;
	} else {
		dbg_error("invalid input (received %d arguments, expected %d or none)\n", argc, ARGC_CNT);
		return -1;
	}

	sim_remove_old_output_files(p_env->paths);

	p_env->mem.data = mem_alloc(MEM_LEN);
	if (!p_env->mem.data) {
		sim_cleanup(p_env);
		return -1;
	}

	p_env->mem.dump_path = p_env->paths[PATH_MEMOUT];
	p_env->mem.p_bus = &p_env->bus;
	res = mem_load(p_env->paths[PATH_MEMIN], p_env->mem.data, MEM_LEN, MAIN_MEM_MODE);
	if (res < 0) {
		sim_cleanup(p_env);
		return -1;
	}

	bus_init(&p_env->bus, p_env->paths[PATH_BUSTRACE]);

	for (int i = 0; i < CORE_MAX; i++) {
		res = core_alloc(&p_env->core[i], i);
		if (res < 0) {
			sim_cleanup(p_env);
			return -1;
		}

		res = core_load(&p_env->core[i], p_env->paths, &p_env->bus);
		if (res < 0) {
			sim_cleanup(p_env);
			return -1;
		}
	}

	dbg_info("simulation init success\n");

	return 0;
}

void sim_clock_tick(struct sim_env *p_env)
{
	for (int i = 0; i < CORE_MAX; i++) {
		core_clock_tick(&p_env->core[i]);
	}

	sim_clk++;
}

void sim_snoop(struct sim_env *p_env)
{
	for (int i = 0; i < CORE_MAX; i++) {
		core_snoop1(&p_env->core[i]);
	}

	mem_snoop(&p_env->mem);

	for (int i = 0; i < CORE_MAX; i++) {
		core_snoop2(&p_env->core[i]);
	}
}

void sim_core_cycle(struct sim_env *p_env)
{
	p_env->run = false;

	for (int i = 0; i < CORE_MAX; i++) {
		if (!core_is_done(&p_env->core[i])) {
			core_cycle(&p_env->core[i]);
			p_env->run = true;
		}
	}
}

void sim_run(struct sim_env *p_env)
{
	while (p_env->run) {
		dbg_verbose("[ %d ]: ----------------------\n", sim_clk);
		sim_snoop(p_env);
		sim_core_cycle(p_env);
		bus_trace(&p_env->bus); // FIXME: consider moving to start of loop - clock -1 issue
		sim_clock_tick(p_env);

		if (sim_clk == MAX_ITERATIONS) {
			dbg_info("max iterations!\n");
			break;
		}
	}

	dbg_info("simultaion run done. clk=%d\n", sim_clk);
}

void sim_dump(struct sim_env *p_env)
{
	mem_dump(&p_env->mem);

	for (int i = 0; i < CORE_MAX; i++) {
		core_dump(&p_env->core[i]);
	}
}
