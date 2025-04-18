// SPDX-License-Identifier: CDDL-1.0
/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or https://opensource.org/licenses/CDDL-1.0.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright (c) 2005, 2010, Oracle and/or its affiliates. All rights reserved.
 * Copyright (c) 2012, 2018 by Delphix. All rights reserved.
 */

/* Portions Copyright 2010 Robert Milkowski */

#ifndef	_SYS_ZIL_IMPL_H
#define	_SYS_ZIL_IMPL_H

#include <sys/zil.h>
#include <sys/dmu_objset.h>

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * Possible states for a given lwb structure.
 *
 * An lwb will start out in the "new" state, and transition to the "opened"
 * state via a call to zil_lwb_write_open() on first itx assignment.  When
 * transitioning from "new" to "opened" the zilog's "zl_issuer_lock" must be
 * held.
 *
 * After the lwb is "opened", it can be assigned number of itxs and transition
 * into the "closed" state via zil_lwb_write_close() when full or on timeout.
 * When transitioning from "opened" to "closed" the zilog's "zl_issuer_lock"
 * must be held.  New lwb allocation also takes "zl_lock" to protect the list.
 *
 * After the lwb is "closed", it can transition into the "ready" state via
 * zil_lwb_write_issue().  "zl_lock" must be held when making this transition.
 * Since it is done by the same thread, "zl_issuer_lock" is not needed.
 *
 * When lwb in "ready" state receives its block pointer, it can transition to
 * "issued". "zl_lock" must be held when making this transition.
 *
 * After the lwb's write zio completes, it transitions into the "write
 * done" state via zil_lwb_write_done(); and then into the "flush done"
 * state via zil_lwb_flush_vdevs_done(). When transitioning from
 * "issued" to "write done", and then from "write done" to "flush done",
 * the zilog's "zl_lock" must be held, *not* the "zl_issuer_lock".
 *
 * The zilog's "zl_issuer_lock" can become heavily contended in certain
 * workloads, so we specifically avoid acquiring that lock when
 * transitioning an lwb from "issued" to "done". This allows us to avoid
 * having to acquire the "zl_issuer_lock" for each lwb ZIO completion,
 * which would have added more lock contention on an already heavily
 * contended lock.
 *
 * Additionally, correctness when reading an lwb's state is often
 * achieved by exploiting the fact that these state transitions occur in
 * this specific order; i.e. "new" to "opened" to "closed" to "ready" to
 * "issued" to "write_done" and finally "flush_done".
 *
 * Thus, if an lwb is in the "new" or "opened" state, holding the
 * "zl_issuer_lock" will prevent a concurrent thread from transitioning
 * that lwb to the "closed" state. Likewise, if an lwb is already in the
 * "ready" state, holding the "zl_lock" will prevent a concurrent thread
 * from transitioning that lwb to the "issued" state.
 */
typedef enum {
    LWB_STATE_NEW,
    LWB_STATE_OPENED,
    LWB_STATE_CLOSED,
    LWB_STATE_READY,
    LWB_STATE_ISSUED,
    LWB_STATE_WRITE_DONE,
    LWB_STATE_FLUSH_DONE,
    LWB_NUM_STATES
} lwb_state_t;

/*
 * Log write block (lwb)
 *
 * Prior to an lwb being issued to disk via zil_lwb_write_issue(), it
 * will be protected by the zilog's "zl_issuer_lock". Basically, prior
 * to it being issued, it will only be accessed by the thread that's
 * holding the "zl_issuer_lock". After the lwb is issued, the zilog's
 * "zl_lock" is used to protect the lwb against concurrent access.
 */
typedef struct lwb {
	zilog_t		*lwb_zilog;	/* back pointer to log struct */
	blkptr_t	lwb_blk;	/* on disk address of this log blk */
	boolean_t	lwb_slim;	/* log block has slim format */
	boolean_t	lwb_slog;	/* lwb_blk is on SLOG device */
	int		lwb_error;	/* log block allocation error */
	int		lwb_nmax;	/* max bytes in the buffer */
	int		lwb_nused;	/* # used bytes in buffer */
	int		lwb_nfilled;	/* # filled bytes in buffer */
	int		lwb_sz;		/* size of block and buffer */
	lwb_state_t	lwb_state;	/* the state of this lwb */
	char		*lwb_buf;	/* log write buffer */
	zio_t		*lwb_child_zio;	/* parent zio for children */
	zio_t		*lwb_write_zio;	/* zio for the lwb buffer */
	zio_t		*lwb_root_zio;	/* root zio for lwb write and flushes */
	hrtime_t	lwb_issued_timestamp; /* when was the lwb issued? */
	uint64_t	lwb_issued_txg;	/* the txg when the write is issued */
	uint64_t	lwb_alloc_txg;	/* the txg when lwb_blk is allocated */
	uint64_t	lwb_max_txg;	/* highest txg in this lwb */
	list_node_t	lwb_node;	/* zilog->zl_lwb_list linkage */
	list_node_t	lwb_issue_node;	/* linkage of lwbs ready for issue */
	list_t		lwb_itxs;	/* list of itx's */
	list_t		lwb_waiters;	/* list of zil_commit_waiter's */
	avl_tree_t	lwb_vdev_tree;	/* vdevs to flush after lwb write */
	kmutex_t	lwb_vdev_lock;	/* protects lwb_vdev_tree */
} lwb_t;

/*
 * ZIL commit waiter.
 *
 * This structure is allocated each time zil_commit() is called, and is
 * used by zil_commit() to communicate with other parts of the ZIL, such
 * that zil_commit() can know when it safe for it return. For more
 * details, see the comment above zil_commit().
 *
 * The "zcw_lock" field is used to protect the commit waiter against
 * concurrent access. This lock is often acquired while already holding
 * the zilog's "zl_issuer_lock" or "zl_lock"; see the functions
 * zil_process_commit_list() and zil_lwb_flush_vdevs_done() as examples
 * of this. Thus, one must be careful not to acquire the
 * "zl_issuer_lock" or "zl_lock" when already holding the "zcw_lock";
 * e.g. see the zil_commit_waiter_timeout() function.
 */
typedef struct zil_commit_waiter {
	kcondvar_t	zcw_cv;		/* signalled when "done" */
	kmutex_t	zcw_lock;	/* protects fields of this struct */
	list_node_t	zcw_node;	/* linkage in lwb_t:lwb_waiter list */
	lwb_t		*zcw_lwb;	/* back pointer to lwb when linked */
	boolean_t	zcw_done;	/* B_TRUE when "done", else B_FALSE */
	int		zcw_zio_error;	/* contains the zio io_error value */
} zil_commit_waiter_t;

/*
 * Intent log transaction lists
 */
typedef struct itxs {
	list_t		i_sync_list;	/* list of synchronous itxs */
	avl_tree_t	i_async_tree;	/* tree of foids for async itxs */
} itxs_t;

typedef struct itxg {
	kmutex_t	itxg_lock;	/* lock for this structure */
	uint64_t	itxg_txg;	/* txg for this chain */
	itxs_t		*itxg_itxs;	/* sync and async itxs */
} itxg_t;

/* for async nodes we build up an AVL tree of lists of async itxs per file */
typedef struct itx_async_node {
	uint64_t	ia_foid;	/* file object id */
	list_t		ia_list;	/* list of async itxs for this foid */
	avl_node_t	ia_node;	/* AVL tree linkage */
} itx_async_node_t;

/*
 * Vdev flushing: during a zil_commit(), we build up an AVL tree of the vdevs
 * we've touched so we know which ones need a write cache flush at the end.
 */
typedef struct zil_vdev_node {
	uint64_t	zv_vdev;	/* vdev to be flushed */
	avl_node_t	zv_node;	/* AVL tree linkage */
} zil_vdev_node_t;

#define	ZIL_BURSTS 8

/*
 * Stable storage intent log management structure.  One per dataset.
 */
struct zilog {
	kmutex_t	zl_lock;	/* protects most zilog_t fields */
	struct dsl_pool	*zl_dmu_pool;	/* DSL pool */
	spa_t		*zl_spa;	/* handle for read/write log */
	const zil_header_t *zl_header;	/* log header buffer */
	objset_t	*zl_os;		/* object set we're logging */
	zil_get_data_t	*zl_get_data;	/* callback to get object content */
	lwb_t		*zl_last_lwb_opened; /* most recent lwb opened */
	hrtime_t	zl_last_lwb_latency; /* zio latency of last lwb done */
	uint64_t	zl_lr_seq;	/* on-disk log record sequence number */
	uint64_t	zl_commit_lr_seq; /* last committed on-disk lr seq */
	uint64_t	zl_destroy_txg;	/* txg of last zil_destroy() */
	uint64_t	zl_replayed_seq[TXG_SIZE]; /* last replayed rec seq */
	uint64_t	zl_replaying_seq; /* current replay seq number */
	uint32_t	zl_suspend;	/* log suspend count */
	kcondvar_t	zl_cv_suspend;	/* log suspend completion */
	uint8_t		zl_suspending;	/* log is currently suspending */
	uint8_t		zl_keep_first;	/* keep first log block in destroy */
	uint8_t		zl_replay;	/* replaying records while set */
	uint8_t		zl_stop_sync;	/* for debugging */
	kmutex_t	zl_issuer_lock;	/* single writer, per ZIL, at a time */
	uint8_t		zl_logbias;	/* latency or throughput */
	uint8_t		zl_sync;	/* synchronous or asynchronous */
	int		zl_parse_error;	/* last zil_parse() error */
	uint64_t	zl_parse_blk_seq; /* highest blk seq on last parse */
	uint64_t	zl_parse_lr_seq; /* highest lr seq on last parse */
	uint64_t	zl_parse_blk_count; /* number of blocks parsed */
	uint64_t	zl_parse_lr_count; /* number of log records parsed */
	itxg_t		zl_itxg[TXG_SIZE]; /* intent log txg chains */
	list_t		zl_itx_commit_list; /* itx list to be committed */
	uint64_t	zl_cur_size;	/* current burst full size */
	uint64_t	zl_cur_left;	/* current burst remaining size */
	uint64_t	zl_cur_max;	/* biggest record in current burst */
	list_t		zl_lwb_list;	/* in-flight log write list */
	avl_tree_t	zl_bp_tree;	/* track bps during log parse */
	clock_t		zl_replay_time;	/* lbolt of when replay started */
	uint64_t	zl_replay_blks;	/* number of log blocks replayed */
	zil_header_t	zl_old_header;	/* debugging aid */
	uint_t		zl_parallel;	/* workload is multi-threaded */
	uint_t		zl_prev_rotor;	/* rotor for zl_prev[] */
	uint_t		zl_prev_opt[ZIL_BURSTS]; /* optimal block size */
	uint_t		zl_prev_min[ZIL_BURSTS]; /* minimal first block size */
	txg_node_t	zl_dirty_link;	/* protected by dp_dirty_zilogs list */
	uint64_t	zl_dirty_max_txg; /* highest txg used to dirty zilog */

	kmutex_t	zl_lwb_io_lock; /* protect following members */
	uint64_t	zl_lwb_inflight[TXG_SIZE]; /* io issued, but not done */
	kcondvar_t	zl_lwb_io_cv;	/* signal when the flush is done */
	uint64_t	zl_lwb_max_issued_txg; /* max txg when lwb io issued */

	/*
	 * Max block size for this ZIL.  Note that this can not be changed
	 * while the ZIL is in use because consumers (ZPL/zvol) need to take
	 * this into account when deciding between WR_COPIED and WR_NEED_COPY
	 * (see zil_max_copied_data()).
	 */
	uint64_t	zl_max_block_size;

	/* Pointer for per dataset zil sums */
	zil_sums_t *zl_sums;
};

typedef struct zil_bp_node {
	dva_t		zn_dva;
	avl_node_t	zn_node;
} zil_bp_node_t;

#ifdef	__cplusplus
}
#endif

#endif	/* _SYS_ZIL_IMPL_H */
