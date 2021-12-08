#include "sim.h"

int main(int argc, char **argv)
{
	struct sim_env env = {0};
	int res;

	res = sim_init(&env, --argc, ++argv);
	if (res < 0) {
		return -1;
	}

	sim_run(&env);
	sim_dump(&env);
	sim_cleanup(&env);

	return 0;
}