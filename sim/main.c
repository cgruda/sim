#include "dbg.h"
#include <stdio.h>
#include <stdint.h>
#include "common.h"
#include "core.h"
#include "mem.h"

#define ARGC_CNT	27

struct sim_env {
	char **paths;

	struct core *core[CORE_MAX];
	uint32_t *mem[CORE_MAX];
};

int sim_cleanup(struct sim_env *p_env)
{
	for (int i = 0; i < CORE_MAX; i++) {
		if (p_env->core[i])
			core_free(p_env->core[i]);
		
		if (p_env->mem[i])
			mem_free(p_env->mem[i]);
	}

	dbg_info("Cleanup done\n");

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

	if (!argc) {
		p_env->paths = (char **)&default_paths;
	} else if (argc == ARGC_CNT) {
		p_env->paths = argv;
	} else {
		dbg_error("invalid input (received %d arguments, expected %d or none)\n", argc, ARGC_CNT);
		return -1;
	}

	sim_remove_old_output_files(p_env->paths);

	for (int i = 0; i < CORE_MAX; i++) {
		p_env->core[i] = core_alloc(i);
		if (!p_env->core[i]) {
			sim_cleanup(p_env);
			return -1;
		}

		// FIXME: temp
		p_env->mem[i] = mem_alloc(MEM_LEN);
		if (!p_env->mem[i]) {
			sim_cleanup(p_env);
			return -1;
		}

		// FIXME: temp
		res = mem_load(p_env->paths[PATH_MEMIN], p_env->mem[i], MEM_LEN);
		if (res < 0) {
			sim_cleanup(p_env);
			return -1;
		}

		res = core_load(p_env->paths, p_env->core[i], p_env->mem[i]);
		if (res < 0) {
			sim_cleanup(p_env);
			return -1;
		}
	}

	dbg_info("Init done\n");

	return 0;
}

void sim_clock_tick(struct sim_env *p_env)
{
	for (int i = 0; i < CORE_MAX; i++)
		core_clock_tick(p_env->core[i]);

	g_clk++;
}

#define ALL_CORES_DONE	(BIT(CORE_MAX) - 1)

void sim_run(struct sim_env *p_env)
{
	uint8_t done_bitmap = 0;

	while (1) {
		for (int i = 0; i < CORE_MAX; i++) {
			if (!core_is_done(p_env->core[i]))
				core_cycle(p_env->core[i]);
			else
				done_bitmap |= BIT(i);
		}

		if (done_bitmap == ALL_CORES_DONE) {
			dbg_info("All cores are done. g_clk=%d\n", g_clk);
			break;
		}

		sim_clock_tick(p_env);
	}
}

void sim_dump(struct sim_env *p_env)
{
	for (int i = 0; i < CORE_MAX; i++) {
		mem_dump(p_env->paths[PATH_MEMOUT], p_env->mem[i], MEM_LEN);
		core_dump_reg(p_env->paths[PATH_REGOUT0], p_env->core[i]);
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
	sim_dump(&env);

	sim_cleanup(&env);

	return 0;
}