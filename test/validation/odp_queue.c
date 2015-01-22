/* Copyright (c) 2014, Linaro Limited
 * All rights reserved.
 *
 * SPDX-License-Identifier:     BSD-3-Clause
 */

#include <odp.h>
#include <odp_cunit_common.h>

#define MAX_BUFFER_QUEUE        (8)
#define MSG_POOL_SIZE           (4*1024*1024)
#define CONFIG_MAX_ITERATION    (100)

static int queue_contest = 0xff;

static int init_queue_suite(void)
{
	odp_buffer_pool_t pool;
	odp_buffer_pool_param_t params;

	params.buf_size  = 0;
	params.buf_align = ODP_CACHE_LINE_SIZE;
	params.num_bufs  = 1024 * 10;
	params.buf_type  = ODP_BUFFER_TYPE_RAW;

	pool = odp_buffer_pool_create("msg_pool", ODP_SHM_NULL, &params);

	if (ODP_BUFFER_POOL_INVALID == pool) {
		printf("Pool create failed.\n");
		return -1;
	}
	return 0;
}

static void test_odp_queue_sunnyday(void)
{
	odp_queue_t queue_creat_id, queue_id;
	odp_buffer_t enbuf[MAX_BUFFER_QUEUE], debuf[MAX_BUFFER_QUEUE];
	odp_buffer_t buf;
	odp_buffer_pool_t msg_pool;
	odp_queue_param_t param;
	odp_buffer_t *pbuf_tmp;
	int i, deq_ret;
	int nr_deq_entries = 0;
	int max_iteration = CONFIG_MAX_ITERATION;
	void *prtn = NULL;

	memset(&param, 0, sizeof(param));
	param.sched.sync  = ODP_SCHED_SYNC_NONE;

	queue_creat_id = odp_queue_create("test_queue",
					  ODP_QUEUE_TYPE_POLL, &param);
	CU_ASSERT(ODP_QUEUE_INVALID != queue_creat_id);

	CU_ASSERT_EQUAL(ODP_QUEUE_TYPE_POLL,
			odp_queue_type(queue_creat_id));
	CU_ASSERT_EQUAL(ODP_SCHED_SYNC_NONE,
			odp_queue_sched_type(queue_creat_id));

	queue_id = odp_queue_lookup("test_queue");
	CU_ASSERT_EQUAL(queue_creat_id, queue_id);

	CU_ASSERT(0 == odp_queue_set_context(queue_id, &queue_contest));

	prtn = odp_queue_get_context(queue_id);
	CU_ASSERT(&queue_contest == (int *)prtn);

	msg_pool = odp_buffer_pool_lookup("msg_pool");
	buf = odp_buffer_alloc(msg_pool);

	odp_queue_enq(queue_id, buf);
	CU_ASSERT_EQUAL(buf, odp_queue_deq(queue_id));
	odp_buffer_free(buf);

	for (i = 0; i < MAX_BUFFER_QUEUE; i++)
		enbuf[i] = odp_buffer_alloc(msg_pool);

	/*
	 * odp_queue_enq_multi may return 0..n buffers due to the resource
	 * constraints in the implementation at that given point of time.
	 */
	odp_queue_enq_multi(queue_id, enbuf, MAX_BUFFER_QUEUE);
	pbuf_tmp = debuf;
	do {
		deq_ret  = odp_queue_deq_multi(queue_id, pbuf_tmp,
					       MAX_BUFFER_QUEUE);
		nr_deq_entries += deq_ret;
		max_iteration--;
		pbuf_tmp += deq_ret;
		CU_ASSERT(max_iteration >= 0);
	} while (nr_deq_entries < MAX_BUFFER_QUEUE);

	for (i = 0; i < MAX_BUFFER_QUEUE; i++) {
		CU_ASSERT_EQUAL(enbuf[i], debuf[i]);
		odp_buffer_free(enbuf[i]);
	}

	return;
}

CU_TestInfo test_odp_queue[] = {
	{"queue sunnyday",  test_odp_queue_sunnyday},
	CU_TEST_INFO_NULL,
};

CU_SuiteInfo odp_testsuites[] = {
	{"Queue", init_queue_suite, NULL, NULL, NULL, test_odp_queue},
	CU_SUITE_INFO_NULL,
};
