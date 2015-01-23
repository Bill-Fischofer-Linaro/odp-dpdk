/* Copyright (c) 2013, Linaro Limited
 * All rights reserved.
 *
 * SPDX-License-Identifier:     BSD-3-Clause
 */

#include <odp_std_types.h>
#include <odp_buffer_pool.h>
#include <odp_buffer_pool_internal.h>
#include <odp_buffer_internal.h>
#include <odp_packet_internal.h>
#include <odp_timer_internal.h>
#include <odp_align_internal.h>
#include <odp_shared_memory.h>
#include <odp_align.h>
#include <odp_internal.h>
#include <odp_config.h>
#include <odp_hints.h>
#include <odp_debug.h>
#include <odp_debug_internal.h>

#include <string.h>
#include <stdlib.h>

/* for DPDK */
#include <odp_packet_dpdk.h>

#define MBUF_SIZE (2048 + sizeof(struct rte_mbuf) + RTE_PKTMBUF_HEADROOM)
#define NB_MBUF   32768

#ifdef POOL_USE_TICKETLOCK
#include <odp_ticketlock.h>
#define LOCK(a)      odp_ticketlock_lock(a)
#define UNLOCK(a)    odp_ticketlock_unlock(a)
#define LOCK_INIT(a) odp_ticketlock_init(a)
#else
#include <odp_spinlock.h>
#define LOCK(a)      odp_spinlock_lock(a)
#define UNLOCK(a)    odp_spinlock_unlock(a)
#define LOCK_INIT(a) odp_spinlock_init(a)
#endif


#if ODP_CONFIG_BUFFER_POOLS > ODP_BUFFER_MAX_POOLS
#error ODP_CONFIG_BUFFER_POOLS > ODP_BUFFER_MAX_POOLS
#endif

#define NULL_INDEX ((uint32_t)-1)

union buffer_type_any_u {
	odp_buffer_hdr_t  buf;
	odp_packet_hdr_t  pkt;
	odp_timeout_hdr_t tmo;
};

typedef union buffer_type_any_u odp_any_buffer_hdr_t;

typedef struct pool_table_t {
	pool_entry_t pool[ODP_CONFIG_BUFFER_POOLS];

} pool_table_t;


/* The pool table ptr - resides in shared memory */
static pool_table_t *pool_tbl;

/* Pool entry pointers (for inlining) */
void *pool_entry_ptr[ODP_CONFIG_BUFFER_POOLS];


int odp_buffer_pool_init_global(void)
{
	odp_buffer_pool_t i;
	odp_shm_t shm;

	shm = odp_shm_reserve("odp_buffer_pools",
			      sizeof(pool_table_t),
			      sizeof(pool_entry_t), 0);

	pool_tbl = odp_shm_addr(shm);

	if (pool_tbl == NULL)
		return -1;

	memset(pool_tbl, 0, sizeof(pool_table_t));


	for (i = 0; i < ODP_CONFIG_BUFFER_POOLS; i++) {
		/* init locks */
		pool_entry_t *pool = &pool_tbl->pool[i];
		LOCK_INIT(&pool->s.lock);
		pool->s.pool = i;

		pool_entry_ptr[i] = pool;
	}

	ODP_DBG("\nBuffer pool init global\n");
	ODP_DBG("  pool_entry_s size     %zu\n", sizeof(struct pool_entry_s));
	ODP_DBG("  pool_entry_t size     %zu\n", sizeof(pool_entry_t));
	ODP_DBG("  odp_buffer_hdr_t size %zu\n", sizeof(odp_buffer_hdr_t));
	ODP_DBG("\n");

	return 0;
}

struct mbuf_ctor_arg {
	uint16_t seg_buf_offset; /* To skip the ODP buf/pkt/tmo header */
	uint16_t seg_buf_size;   /* total sz: offset + user sz + HDROOM */
	int buf_type;
};

struct mbuf_pool_ctor_arg {
	uint16_t seg_buf_size; /* size of mbuf: user specified sz + HDROOM */
};

static void
odp_dpdk_mbuf_pool_ctor(struct rte_mempool *mp,
			void *opaque_arg)
{
	struct mbuf_pool_ctor_arg      *mbp_ctor_arg;
	struct rte_pktmbuf_pool_private *mbp_priv;

	if (mp->private_data_size < sizeof(struct rte_pktmbuf_pool_private)) {
		ODP_ERR("%s(%s) private_data_size %d < %d",
			__func__, mp->name, (int) mp->private_data_size,
			(int) sizeof(struct rte_pktmbuf_pool_private));
		return;
	}
	mbp_ctor_arg = (struct mbuf_pool_ctor_arg *)opaque_arg;
	mbp_priv = rte_mempool_get_priv(mp);
	mbp_priv->mbuf_data_room_size = mbp_ctor_arg->seg_buf_size;
}

/* ODP DPDK mbuf constructor.
 * This is a combination of rte_pktmbuf_init in rte_mbuf.c
 * and testpmd_mbuf_ctor in testpmd.c
 */
static void
odp_dpdk_mbuf_ctor(struct rte_mempool *mp,
		   void *opaque_arg,
		   void *raw_mbuf,
		   unsigned i)
{
	struct mbuf_ctor_arg *mb_ctor_arg;
	struct rte_mbuf *mb = raw_mbuf;
	struct odp_buffer_hdr_t *buf_hdr;

	/* The rte_mbuf is at the begninning in all cases */
	mb_ctor_arg = (struct mbuf_ctor_arg *)opaque_arg;
	mb = (struct rte_mbuf *)raw_mbuf;

	RTE_MBUF_ASSERT(mp->elt_size >= sizeof(struct rte_mbuf));

	memset(mb, 0, mp->elt_size);

	/* Start of buffer is just after the ODP type specific header
	 * which contains in the very beginning the rte_mbuf struct */
	mb->buf_addr     = (char *)mb + mb_ctor_arg->seg_buf_offset;
	mb->buf_physaddr = rte_mempool_virt2phy(mp, mb) +
			mb_ctor_arg->seg_buf_offset;
	mb->buf_len      = mb_ctor_arg->seg_buf_size;

	/* keep some headroom between start of buffer and data */
	if (mb_ctor_arg->buf_type == ODP_BUFFER_TYPE_PACKET ||
	    mb_ctor_arg->buf_type == ODP_BUFFER_TYPE_ANY)
		mb->pkt.data = (char *)mb->buf_addr + RTE_PKTMBUF_HEADROOM;
	else
		mb->pkt.data = mb->buf_addr;

	/* init some constant fields */
	mb->type         = RTE_MBUF_PKT;
	mb->pool         = mp;
	mb->pkt.nb_segs  = 1;
	mb->pkt.in_port  = 0xff;
	mb->ol_flags     = 0;
	mb->pkt.vlan_macip.data = 0;
	mb->pkt.hash.rss = 0;

	/* Save index, might be useful for debugging purposes */
	buf_hdr = (struct odp_buffer_hdr_t *)raw_mbuf;
	buf_hdr->index = i;
}

odp_buffer_pool_t odp_buffer_pool_create(const char *name,
					 odp_shm_t shm,
					 odp_buffer_pool_param_t *params)
{
	struct rte_mempool *pool = NULL;
	struct mbuf_pool_ctor_arg mbp_ctor_arg;
	struct mbuf_ctor_arg mb_ctor_arg;
	unsigned mb_size;
	size_t hdr_size;

	ODP_DBG("odp_buffer_pool_create: %s, %u, %u, %u, %d\n", name,
		params->num_bufs, params->buf_size, params->buf_align,
		params->buf_type);

	switch (params->buf_type) {
	case ODP_BUFFER_TYPE_RAW:
		hdr_size = sizeof(odp_buffer_hdr_t);
		mbp_ctor_arg.seg_buf_size = (uint16_t) params->buf_size;
		break;
	case ODP_BUFFER_TYPE_PACKET:
		hdr_size = sizeof(odp_packet_hdr_t);
		mbp_ctor_arg.seg_buf_size =
			(uint16_t) (RTE_PKTMBUF_HEADROOM + params->buf_size);
		break;
	case ODP_BUFFER_TYPE_TIMEOUT:
		hdr_size = sizeof(odp_timeout_hdr_t);
		mbp_ctor_arg.seg_buf_size = (uint16_t) params->buf_size;
		break;
	case ODP_BUFFER_TYPE_ANY:
		hdr_size = sizeof(odp_any_buffer_hdr_t);
		mbp_ctor_arg.seg_buf_size =
			(uint16_t) (RTE_PKTMBUF_HEADROOM + params->buf_size);
		break;
	default:
		ODP_ERR("odp_buffer_pool_create: Bad type %i\n",
			params->buf_type);
		return ODP_BUFFER_POOL_INVALID;
		break;
	}

	mb_ctor_arg.seg_buf_offset =
		(uint16_t) ODP_CACHE_LINE_SIZE_ROUNDUP(hdr_size);
	mb_ctor_arg.seg_buf_size = mbp_ctor_arg.seg_buf_size;
	mb_ctor_arg.buf_type = params->buf_type;
	mb_size = mb_ctor_arg.seg_buf_offset + mb_ctor_arg.seg_buf_size;

	pool = rte_mempool_create(name, NB_MBUF,
				  mb_size, MAX_PKT_BURST,
				  sizeof(struct rte_pktmbuf_pool_private),
				  odp_dpdk_mbuf_pool_ctor, &mbp_ctor_arg,
				  odp_dpdk_mbuf_ctor, &mb_ctor_arg,
				  rte_socket_id(), 0);
	if (pool == NULL) {
		ODP_ERR("Cannot init DPDK mbuf pool\n");
		return ODP_BUFFER_POOL_INVALID;
	}

	return (odp_buffer_pool_t) pool;
}


odp_buffer_pool_t odp_buffer_pool_lookup(const char *name)
{
	struct rte_mempool *mp = NULL;

	mp = rte_mempool_lookup(name);
	if (mp == NULL)
		return ODP_BUFFER_POOL_INVALID;

	return (odp_buffer_pool_t)mp;
}


odp_buffer_t odp_buffer_alloc(odp_buffer_pool_t pool_id)
{
	return (odp_buffer_t)rte_pktmbuf_alloc((struct rte_mempool *)pool_id);
}


void odp_buffer_free(odp_buffer_t buf)
{
	rte_pktmbuf_free((struct rte_mbuf *)buf);
}


void odp_buffer_pool_print(odp_buffer_pool_t pool_id)
{
	rte_mempool_dump(stdout, (const struct rte_mempool *)pool_id);
}
