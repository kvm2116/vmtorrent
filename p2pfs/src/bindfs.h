#ifndef _BINDFS_H_
#define _BINDFS_H_

#include <stdio.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <fuse.h>
#include <fuse_opt.h>
#include "../p2p/p2p_wrapper.hpp"

#define DEFAULT_LOG "/tmp/log"
#define DEFAULT_STORAGE "/tmp"

extern FILE *logfile;
extern double read_delay; 
extern double write_delay; 
extern int r_accesses;
extern int w_accesses;
extern struct timeval starttime;
extern int blocks_per_piece;
extern pthread_mutex_t print_lock;

struct bitmap {
    off_t size;
    int seqno;
    ino_t inode;
    int resize;
    pthread_mutex_t mutex;
    struct bitmap *next;
    char *map;
};

struct bindfs {
    unsigned long fh;
    struct bitmap *bitmap;
    FILE *logfile;
    int seqno;			/* copy of bitmap->seq */
    void *p2p;			/* copy of the p2p handle */
    char *path;
    struct bindfs *prev;
    struct bindfs *next;
};

/*
 * a bit-torrent block is composed of @nsegs contiguous segments,
 * whose starting fs-block numbers are stored in @blocks array.
 * for example, for nsegs == BT_BLOCK_SIZE/FS_BLOCK_SIZE here is
 * max granularity and flexibility. settings nsegs = 1 is useful
 * when the mapping is simply contiguous.
 */
struct bt_block {
    char *data;
    int nsegs;
    int blocks[BT_BLOCK_SIZE/FS_BLOCK_SIZE];
};

/* interface to block management */
extern void bh_init_blocks(char *torrent);
extern int bh_bring_block(struct bindfs *bh, off_t pos, int rw);
/* simple pos_to_block */
static inline int bh_pos_to_block(off_t pos)
{
    return (pos >> BT_BLOCK_BITS) << (BT_BLOCK_BITS - FS_BLOCK_BITS);
}
/* simple block_to_pos */
static inline off_t bh_block_to_pos(int block)
{
    return ((off_t)block >> (BT_BLOCK_BITS - FS_BLOCK_BITS)) << BT_BLOCK_BITS;
}
/* simple pos_to_piece */
static inline int bh_pos_to_piece(off_t pos)
{
    return bh_pos_to_block(pos) / blocks_per_piece;
}


/* helpful macros for logging */
#define RW_STR          ( rw ? ( rw==1 ? "write" : "push" ) : "read" )
#define TIME_FMT	"%8ld.%06ld"
#define TIME_VAR(tv)	tv.tv_sec, tv.tv_usec

#define POS_FMT		"%ju (%#jx)"
#define POSW_FMT	"%12ju (%#8jx)"
#define POS_VAR(pos)	pos, pos

#define TIME_POS_FMT	TIME_FMT " " POS_FMT
#define TIME_POSW_FMT	TIME_FMT " " POSW_FMT

#define bh_log(bh, fmt, args...)					\
    do {								\
      fprintf((bh)->logfile, "[%03d] " fmt,				\
	      (bh)->seqno, ##args);					\
      fflush((bh)->logfile);						\
    } while (0)

#define STAMP_FMT "\t" __FILE__ ":"  __LINE__ ":" __func__ ": "

#define BINDFS_OUT(fmt, args...)					\
	{								\
		struct timeval tv; pthread_t tid; int nice;		\
		tid = pthread_self();					\
		nice = getpriority (PRIO_PROCESS, 0);			\
		gettimeofday(&tv, NULL);				\
		pthread_mutex_lock(&print_lock);			\
		fprintf(logfile, TIME_FMT ":%p:%d:%s:%d:%s:\t" fmt,	\
			TIME_VAR(tv),(void *)tid, nice,			\
			__FILE__, __LINE__, __func__, ##args);		\
		pthread_mutex_unlock(&print_lock);			\
	}

#define BINDFS_OUT2(tv, fmt, args...)					\
	{								\
		pthread_t tid; int nice;				\
		tid = pthread_self();					\
		nice = getpriority (PRIO_PROCESS, 0);			\
		pthread_mutex_lock(&print_lock);			\
		fprintf(logfile, TIME_FMT ":%p:%d:%s:%d:%s:\t" fmt,	\
			TIME_VAR(tv),(void *)tid, nice,			\
			__FILE__, __LINE__, __func__, ##args);		\
		pthread_mutex_unlock(&print_lock);			\
	}


#define FS_PTHREAD_MUTEX_LOCK(lock)					\
	{								\
		pthread_mutex_lock(lock);				\
	}		

#define FS_PTHREAD_MUTEX_LOCK2(lock, block)				\
	{								\
		pthread_mutex_lock(lock);				\
	}

/*
#define FS_PTHREAD_MUTEX_LOCK(lock)					\
	{								\
		struct timeval tv; double t_start, t_end;		\
		gettimeofday(&tv, NULL);				\
		t_start = (double)tv.tv_sec+(double)tv.tv_usec/1000000; \
		pthread_mutex_lock(lock);				\
		gettimeofday(&tv, NULL);				\
		t_end = (double)tv.tv_sec + (double)tv.tv_usec/1000000;	\
		BINDFS_OUT("obtained, lock=%p, delay=%f\n",		\
			   lock, t_end-t_start);			\
	}		

#define FS_PTHREAD_MUTEX_LOCK2(lock, block)				\
	{								\
		struct timeval tv; double t_start, t_end;		\
		gettimeofday(&tv, NULL);				\
		t_start = (double)tv.tv_sec+(double)tv.tv_usec/1000000; \
		pthread_mutex_lock(lock);				\
		gettimeofday(&tv, NULL);				\
		t_end = (double)tv.tv_sec + (double)tv.tv_usec/1000000;	\
		BINDFS_OUT("obtained, lock=%p, block=%d, delay=%f\n",	\
			   lock, block, t_end-t_start);			\
	}
*/

/* useful macros */
#define SET_TIMEVAL(a, b)  a.tv_sec = b.tv_sec; a.tv_usec = b.tv_usec

#ifndef MIN
#define MIN(a, b)	((a) < (b) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a, b)	((a) > (b) ? (a) : (b))
#endif

#endif /* _BINDFS_H_ */
