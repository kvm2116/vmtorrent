

/* needed for pread/pwrite */
#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <fuse.h>
#include <fuse_opt.h>

#include "bindfs.h"

static int (*bh_get_block)(struct bindfs *bh, struct bt_block *bt, int block);
static void (*bh_put_block)(struct bt_block *bt);

/*
 * Dummy get_block/put_block pair: bring data from a shadow file
 * These provide the blocks of the file from a shadow source files
 * that resides on the filesystem. This is intended to be used for
 * testing.
 */

static int bh_get_block_fs(struct bindfs *bh, struct bt_block *bt, int block)
{
    char *buf;
    char *path;
    int fd, res;
    off_t pos;

#define BH_SUFFIX ".src"

    path = malloc(strlen(bh->path) + strlen(BH_SUFFIX) + 1);
    if (path == NULL)
        return -errno;
    
    sprintf(path, "%s%s", bh->path, BH_SUFFIX);

    buf = malloc(BT_BLOCK_SIZE);
    if (buf == NULL) {
	res = -errno;
	free(path);
	return res;
    }

    memset(buf, 0, BT_BLOCK_SIZE);

    fd = open(path, O_RDONLY, 0);
    if (fd < 0) {
	res = -errno;
	free(path);
	free(buf);
	return res;
    }

    pos = block;
    pos = pos << FS_BLOCK_BITS;
    res = pread(fd, buf, BT_BLOCK_SIZE, pos);
    if (res < 0) {
	res = -errno;
	free(path);
	free(buf);
	close(fd);
	printf("read fail %s\n", strerror(errno));
	return res;
    }

    /*
     * if we hit end-of-file then the shadow file was exhausted,
     * so the default behavior is to return a whole block of zeros
     */

    close(fd);

    bt->data = buf;
    bt->nsegs = 1;
    bt->blocks[0] = block;

    return 0;
}

static inline void bh_put_block_fs(struct bt_block *bt)
{
    free(bt->data);
}


/*
 * Bittorrent get_block/put_block pair: bring data from bittorrent
 * These provide the blocks of the file from the bitorrent clients.
 */

static int bh_get_block_bt(struct bindfs *bh, struct bt_block *bt, int block)
{
    char *buf;
    int blk, res;

    buf = malloc(BT_BLOCK_SIZE);
    if (buf == NULL)
	    return -errno;

    memset(buf, 0, BT_BLOCK_SIZE);
    blk = block >> (BT_BLOCK_BITS - FS_BLOCK_BITS);
    res = p2p_c_getblock(bh->p2p, blk, buf, BT_BLOCK_SIZE);
 
    if (res < 0) {
	    free(buf);
	    errno = EIO;
	    return -EIO;
    }

    bt->data = buf;
    bt->nsegs = 1;
    bt->blocks[0] = block;

    return 0;
}

static inline void bh_put_block_bt(struct bt_block *bt)
{
    free(bt->data);
}

/* logic that decides which blocks to bring and what to do with them */

void bh_init_blocks(char *torrent)
{
    if (torrent) {
	bh_get_block = bh_get_block_bt;
	bh_put_block = bh_put_block_bt;
    } else {
	bh_get_block = bh_get_block_fs;
	bh_put_block = bh_put_block_fs;
    }
}


//TODO OPTIMIZE FOR PIECES COMPRISING CONSECUTIVE BLOCKS
/* bring a block from the bt-client - the bitmap mutex is taken */
int bh_bring_block(struct bindfs *bh, off_t pos, int rw)
{
    struct bt_block bt;
    struct bitmap *bitmap;
    int i, j, bs, size;
    int ind, bit;
    char *buf;
    int res;
    int block;

    block = bh_pos_to_block(pos);
    BINDFS_OUT("start,\ttype=%s,\tblock=%d\n", RW_STR, block);	

    bitmap = bh->bitmap;
    pos &= ~FS_BLOCK_MASK;
    //BINDFS_OUT("check mask,\ttype=%s,\tblock=%d,\tmasked_block=%d\n", RW_STR, block, bh_pos_to_block(pos));	
    
    memset(&bt, 0, sizeof(bt));  /* or gcc warns that it's uninitialized */

    /* temporarily drop the lock to allow concurrency */
    pthread_mutex_unlock(&bitmap->mutex);
    //BINDFS_OUT("*bh_get_block start,\ttype=%s,\tblock=%d\n", RW_STR, block);	
    res = (*bh_get_block)(bh, &bt, block);
    FS_PTHREAD_MUTEX_LOCK2(&bitmap->mutex,block);
    //BINDFS_OUT("*bh_get_block end,\ttype=%s,\tblock=%d\n", RW_STR, block);	
    if (res < 0){
        BINDFS_OUT("*bh_get_block,\terror=%d,\ttype=%s,\tblock=%d\n", res, RW_STR, block);	
	return res;
    }

    bs = BT_BLOCK_SIZE / bt.nsegs;
 

    for (i = 0; i < bt.nsegs; i++) {
	buf = &bt.data[i * bs];
	pos = bt.blocks[i];
	pos = pos << FS_BLOCK_BITS;
	ind = bt.blocks[i] >> 3;
	bit = bt.blocks[i] & 0x7;

	/*
	 * must not copy the segment as is - it may be that individual
	 * blocks in it have already been brought before or dirtied by
	 * the client and we may not overwrite them.
	 */
	for (j = 0; j < bs / FS_BLOCK_SIZE; j++) {
	    if (!(bitmap->map[ind] & (1 << bit))) {
		/* don't copy beyond current end of file */
		size = MIN(bitmap->size - pos, FS_BLOCK_SIZE);
       	        //rw ? w_accesses++ : r_accesses++;
		if (size > 0) {
		    res = pwrite((int) bh->fh, buf, size, pos);
		    if (res < 0){
		      BINDFS_OUT("error=%s,\ttype=%s,\tblock=%d\n", strerror(errno), RW_STR, block);		
		      printf("pwrite failed: %s\n", strerror(errno));
		      break;
		    }
		}
		bitmap->map[ind] |= (1 << bit);
	    }

	    buf += FS_BLOCK_SIZE;
	    pos += FS_BLOCK_SIZE;

	    bit += 1;
	    if (bit == 8) {
		bit = 0;
		ind++;
	    }
	}

	if (res < 0)
	    break;
    }

    (*bh_put_block)(&bt);

    BINDFS_OUT("end,\ttype=%s,\tblock=%d\n", RW_STR, block);		

    return res;
}
