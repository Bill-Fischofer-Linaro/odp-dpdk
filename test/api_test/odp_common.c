/* Copyright (c) 2013, Linaro Limited
 * All rights reserved.
 *
 * SPDX-License-Identifier:     BSD-3-Clause
 */

/**
 * @file
 *
 * ODP test application common
 */

#include <string.h>
#include <odp.h>
#include <odph_linux.h>
#include <odp_common.h>
#include <odp_atomic_test.h>
#include <odp_shm_test.h>
#include <test_debug.h>


/* Globals */
static odph_linux_pthread_t thread_tbl[MAX_WORKERS]; /**< worker threads table*/
static int num_workers;				    /**< number of workers 	*/
__thread test_shared_data_t *test_shared_data;	    /**< pointer to shared data */

/**
 * Print system information
 */
void odp_print_system_info(void)
{
	odp_coremask_t coremask;
	char str[32];

	memset(str, 1, sizeof(str));

	odp_coremask_zero(&coremask);

	odp_coremask_from_str("0x1", &coremask);
	odp_coremask_to_str(str, sizeof(str), &coremask);

	printf("\n");
	printf("ODP system info\n");
	printf("---------------\n");
	printf("ODP API version: %s\n",        odp_version_api_str());
	printf("CPU model:       %s\n",        odp_sys_cpu_model_str());
	printf("CPU freq (hz):   %"PRIu64"\n", odp_sys_cpu_hz());
	printf("Cache line size: %i\n",        odp_sys_cache_line_size());
	printf("Core count:      %i\n",        odp_sys_core_count());
	printf("Core mask:       %s\n",        str);

	printf("\n");
}

/** test init globals and call odp_init_global() */
int odp_test_global_init(void)
{
	memset(thread_tbl, 0, sizeof(thread_tbl));

	if (odp_init_global(NULL, NULL)) {
		LOG_ERR("ODP global init failed.\n");
		return -1;
	}

	num_workers = odp_sys_core_count();
	/* force to max core count */
	if (num_workers > MAX_WORKERS)
		num_workers = MAX_WORKERS;

	return 0;
}

/** create test thread */
int odp_test_thread_create(void *func_ptr(void *), pthrd_arg *arg)
{
	/* Create and init additional threads */
	odph_linux_pthread_create(thread_tbl, arg->numthrds, 0, func_ptr,
				  (void *)arg);

	return 0;
}

/** exit from test thread */
int odp_test_thread_exit(pthrd_arg *arg)
{
	/* Wait for other threads to exit */
	odph_linux_pthread_join(thread_tbl, arg->numthrds);

	return 0;
}
