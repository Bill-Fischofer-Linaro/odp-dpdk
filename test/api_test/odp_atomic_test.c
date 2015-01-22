/* Copyright (c) 2013, Linaro Limited
 * All rights reserved.
 *
 * SPDX-License-Identifier:     BSD-3-Clause
 */

#include <string.h>
#include <sys/time.h>
#include <odp_common.h>
#include <odp_atomic_test.h>
#include <test_debug.h>

static odp_atomic_u32_t a32u;
static odp_atomic_u64_t a64u;

static odp_barrier_t barrier;

static const char * const test_name[] = {
	"dummy",
	"test atomic all (add/sub/inc/dec) on 32- and 64-bit atomic ints",
	"test atomic inc/dec of 32-bit atomic int",
	"test atomic add/sub of 32-bit atomic int",
	"test atomic inc/dec of 64-bit atomic int",
	"test atomic add/sub of 64-bit atomic int"
};

static struct timeval tv0[MAX_WORKERS], tv1[MAX_WORKERS];

static void usage(void)
{
	printf("\n./odp_atomic -t <testcase> [-n <numthreads>]\n\n"
	       "\t<testcase> is\n"
	       "\t\t1 - Test all (inc/dec/add/sub on 32/64-bit atomic ints)\n"
	       "\t\t2 - Test inc/dec of 32-bit atomic int\n"
	       "\t\t3 - Test add/sub of 32-bit atomic int\n"
	       "\t\t4 - Test inc/dec of 64-bit atomic int\n"
	       "\t\t5 - Test add/sub of 64-bit atomic int\n"
	       "\t\t-n <1 - 31> - no of threads to start\n"
	       "\t\tif user doesn't specify this option, then\n"
	       "\t\tno of threads created is equivalent to no of cores\n"
	       "\t\tavailable in the system\n"
	       "\tExample usage:\n"
	       "\t\t./odp_atomic -t 2\n"
	       "\t\t./odp_atomic -t 3 -n 12\n");
}


void test_atomic_inc_u32(void)
{
	int i;

	for (i = 0; i < CNT; i++)
		odp_atomic_inc_u32(&a32u);
}

void test_atomic_inc_64(void)
{
	int i;

	for (i = 0; i < CNT; i++)
		odp_atomic_inc_u64(&a64u);
}

void test_atomic_dec_u32(void)
{
	int i;

	for (i = 0; i < CNT; i++)
		odp_atomic_dec_u32(&a32u);
}

void test_atomic_dec_64(void)
{
	int i;

	for (i = 0; i < CNT; i++)
		odp_atomic_dec_u64(&a64u);
}

void test_atomic_add_u32(void)
{
	int i;

	for (i = 0; i < (CNT / ADD_SUB_CNT); i++)
		odp_atomic_fetch_add_u32(&a32u, ADD_SUB_CNT);
}

void test_atomic_add_64(void)
{
	int i;

	for (i = 0; i < (CNT / ADD_SUB_CNT); i++)
		odp_atomic_fetch_add_u64(&a64u, ADD_SUB_CNT);
}

void test_atomic_sub_u32(void)
{
	int i;

	for (i = 0; i < (CNT / ADD_SUB_CNT); i++)
		odp_atomic_fetch_sub_u32(&a32u, ADD_SUB_CNT);
}

void test_atomic_sub_64(void)
{
	int i;

	for (i = 0; i < (CNT / ADD_SUB_CNT); i++)
		odp_atomic_fetch_sub_u64(&a64u, ADD_SUB_CNT);
}

void test_atomic_inc_dec_u32(void)
{
	test_atomic_inc_u32();
	test_atomic_dec_u32();
}

void test_atomic_add_sub_u32(void)
{
	test_atomic_add_u32();
	test_atomic_sub_u32();
}

void test_atomic_inc_dec_64(void)
{
	test_atomic_inc_64();
	test_atomic_dec_64();
}

void test_atomic_add_sub_64(void)
{
	test_atomic_add_64();
	test_atomic_sub_64();
}

/**
 * Test basic atomic operation like
 * add/sub/increment/decrement operation.
 */
void test_atomic_basic(void)
{
	test_atomic_inc_u32();
	test_atomic_dec_u32();
	test_atomic_add_u32();
	test_atomic_sub_u32();

	test_atomic_inc_64();
	test_atomic_dec_64();
	test_atomic_add_64();
	test_atomic_sub_64();
}

void test_atomic_init(void)
{
	odp_atomic_init_u32(&a32u, 0);
	odp_atomic_init_u64(&a64u, 0);
}

void test_atomic_store(void)
{
	odp_atomic_store_u32(&a32u, U32_INIT_VAL);
	odp_atomic_store_u64(&a64u, U64_INIT_VAL);
}

int test_atomic_validate(void)
{
	if (odp_atomic_load_u32(&a32u) != U32_INIT_VAL) {
		LOG_ERR("Atomic u32 usual functions failed\n");
		return -1;
	}

	if (odp_atomic_load_u64(&a64u) != U64_INIT_VAL) {
		LOG_ERR("Atomic u64 usual functions failed\n");
		return -1;
	}

	return 0;
}

static void *run_thread(void *arg)
{
	pthrd_arg *parg = (pthrd_arg *)arg;
	int thr;

	thr = odp_thread_id();

	LOG_DBG("Thread %i starts\n", thr);

	/* Wait here until all threads have arrived */
	/* Use multiple barriers to verify that it handles wrap around and
	 * has no race conditions which could be exposed when invoked back-
	 * to-back */
	odp_barrier_wait(&barrier);
	odp_barrier_wait(&barrier);
	odp_barrier_wait(&barrier);
	odp_barrier_wait(&barrier);

	gettimeofday(&tv0[thr], NULL);

	switch (parg->testcase) {
	case TEST_MIX:
		test_atomic_basic();
		break;
	case TEST_INC_DEC_U32:
		test_atomic_inc_dec_u32();
		break;
	case TEST_ADD_SUB_U32:
		test_atomic_add_sub_u32();
		break;
	case TEST_INC_DEC_64:
		test_atomic_inc_dec_64();
		break;
	case TEST_ADD_SUB_64:
		test_atomic_add_sub_64();
		break;
	}
	gettimeofday(&tv1[thr], NULL);
	fflush(NULL);

	printf("Time taken in thread %02d to complete op is %lld usec\n", thr,
	       (tv1[thr].tv_sec - tv0[thr].tv_sec) * 1000000ULL +
	       (tv1[thr].tv_usec - tv0[thr].tv_usec));

	return parg;
}

int main(int argc, char *argv[])
{
	pthrd_arg thrdarg;
	int test_type = 0, pthrdnum = 0, i = 0, cnt = argc - 1;
	char c;
	int result;

	if (argc == 1 || argc % 2 == 0) {
		usage();
		goto err_exit;
	}
	if (odp_test_global_init() != 0)
		goto err_exit;
	odp_print_system_info();

	while (cnt != 0) {
		sscanf(argv[++i], "-%c", &c);
		switch (c) {
		case 't':
			sscanf(argv[++i], "%d", &test_type);
			break;
		case 'n':
			sscanf(argv[++i], "%d", &pthrdnum);
			break;
		default:
			LOG_ERR("Invalid option %c\n", c);
			usage();
			goto err_exit;
		}
		if (test_type < TEST_MIX || test_type > TEST_MAX ||
		    pthrdnum > odp_sys_core_count() || pthrdnum < 0) {
			usage();
			goto err_exit;
		}
		cnt -= 2;
	}

	if (pthrdnum == 0)
		pthrdnum = odp_sys_core_count();

	test_atomic_init();
	test_atomic_store();

	memset(&thrdarg, 0, sizeof(pthrd_arg));
	thrdarg.testcase = test_type;
	thrdarg.numthrds = pthrdnum;

	if ((test_type > 0) && (test_type < TEST_MAX)) {
		printf("%s\n", test_name[test_type]);
	} else {
		LOG_ERR("Invalid test case [%d]\n", test_type);
		usage();
		goto err_exit;
	}
	odp_barrier_init(&barrier, pthrdnum);
	odp_test_thread_create(run_thread, &thrdarg);

	odp_test_thread_exit(&thrdarg);

	result = test_atomic_validate();

	if (result == 0) {
		printf("%s_%d_%d Result:pass\n",
		       test_name[test_type], test_type, pthrdnum);
	} else {
		printf("%s_%d_%d Result:fail\n",
		       test_name[test_type], test_type, pthrdnum);
	}
	return 0;

err_exit:
	return -1;
}
