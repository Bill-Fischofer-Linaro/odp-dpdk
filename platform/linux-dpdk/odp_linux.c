/* Copyright (c) 2013, Linaro Limited
 * All rights reserved.
 *
 * SPDX-License-Identifier:     BSD-3-Clause
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <sched.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include <odph_linux.h>
#include <odp_internal.h>
#include <odp_thread.h>
#include <odp_init.h>
#include <odp_system_info.h>

#include <rte_lcore.h>

typedef struct {
	void *(*start_routine) (void *);
	void *arg;

} odp_start_args_t;


static void *odp_run_start_routine(void *arg)
{
	odp_start_args_t *start_args = arg;

	/* ODP thread local init */
	if (odp_init_local()) {
		ODP_ERR("Local init failed\n");
		return NULL;
	}

	return start_args->start_routine(start_args->arg);
}


void odph_linux_pthread_create(odph_linux_pthread_t *thread_tbl, int num,
		int first_core, void *(*start_routine) (void *), void *arg)
{
	int i;
	cpu_set_t cpu_set;
	odp_start_args_t *start_args;
	int core_count;
	int cpu;

	(void) cpu_set;
	(void) thread_tbl;

	core_count = odp_sys_core_count();

	assert((first_core >= 0) && (first_core < core_count));
	assert((num >= 0) && (num <= core_count));

	for (i = 0; i < num; i++) {
		cpu = (first_core + i) % core_count;

		start_args = malloc(sizeof(odp_start_args_t));
		memset(start_args, 0, sizeof(odp_start_args_t));
		start_args->start_routine = start_routine;
		start_args->arg           = arg;

		/* If not master core */
		if (cpu != 0) {
			rte_eal_remote_launch(
				(int(*)(void *))odp_run_start_routine,
				start_args, cpu);
		} else {
			lcore_config[cpu].ret = (int)(uint64_t)
				odp_run_start_routine(start_args);
			lcore_config[cpu].state = FINISHED;
		}
	}
}


void odph_linux_pthread_join(odph_linux_pthread_t *thread_tbl, int num)
{
	uint32_t lcore_id;

	(void) thread_tbl;
	(void) num;

	RTE_LCORE_FOREACH_SLAVE(lcore_id) {
	if (rte_eal_wait_lcore(lcore_id) < 0)
		return;
	}
}
