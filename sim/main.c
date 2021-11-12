#include "dbg.h"
#include <stdio.h>
#include <stdint.h>
#include "common.h"
#include "core.h"
#include "mem.h"

#define ARGC_CNT	27

struct sim_env {
	char **sim_files_paths;

	struct core *p_core0;
	uint32_t *mem;
};

int sim_cleanup(struct sim_env *p_env)
{
	dbg_trace();

	if (p_env->p_core0)
		core_free(p_env->p_core0);

	if (p_env->mem)
		mem_free(p_env->mem);

	return 0;
}

void sim_remove_old_output_files(char **file_paths)
{
	for (int i = PATH_MEMOUT; i < PATH_MAX; i++)
		remove(file_paths[i]);
}

int sim_init(struct sim_env *p_env, int argc, char **argv)
{
	int res = 0;

	dbg_trace();

	if (!argc) {
		p_env->sim_files_paths = (char **)&default_sim_files_paths;
	} else if (argc == ARGC_CNT) {
		p_env->sim_files_paths = argv;
	} else {
		dbg_error("invalid input (received %d arguments, expected %d or none)\n", argc, ARGC_CNT);
		return -1;
	}

	sim_remove_old_output_files(p_env->sim_files_paths);

	p_env->p_core0 = core_init(0);
	if (!p_env->p_core0)
		return -1;

	p_env->mem = mem_init(MEM_LEN);
	if (!p_env->mem) {
		sim_cleanup(p_env);
		return -1;
	}

	res = core_load(p_env->sim_files_paths, p_env->p_core0, p_env->mem);
	if (res < 0) {
		sim_cleanup(p_env);
		return -1;
	}

	res = mem_load(p_env->sim_files_paths[PATH_MEMIN], p_env->mem, MEM_LEN);
	if (res < 0) {
		sim_cleanup(p_env);
		return -1;
	}

	return 0;
}

void sim_clock_tick(struct sim_env *p_env)
{
	core_clock_tick(p_env->p_core0);
	g_clk++;
}

void sim_run(struct sim_env *p_env)
{
	dbg_trace();

	while (!core_is_halt(p_env->p_core0)) {
		core_cycle(p_env->p_core0);
		sim_clock_tick(p_env);
	}
}

int main(int argc, char **argv)
{
	struct sim_env env = {0};
	int res;

	res = sim_init(&env, --argc, ++argv);
	if (res < 0)
		return -1;

	sim_run(&env);

	sim_cleanup(&env);

	return 0;
}