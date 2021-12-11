#include "dbg.h"
#include "sim.h"

dbg_fp_declare()

int main(int argc, char **argv)
{
	dbg_fp_open();
	struct sim_env env = {0};
	int res;

	res = sim_init(&env, --argc, ++argv);
	if (res < 0) {
		return -1;
	}

	sim_run(&env);
	sim_dump(&env);
	sim_cleanup(&env);
	dbg_fp_close();
	return 0;
}
