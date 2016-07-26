#ifndef  __VMTORRENT_DEFINES_HPP_ 
#define  __VMTORRENT_DEFINES_HPP_  1

#include <sys/resource.h>

//I haven't figured out how to make this a conditional compile
//so it always compiles and I determine whether to execute this
//code at runtime, based on the result of p2p_c_register_push
#define VMTORRENT_PUSH_FS 
#define VMTORRENT_PUSH_BLOCKING

/* bit-torrent block size */
#define BT_BLOCK_BITS		14	/* 2^14 = 16KB */
#define BT_BLOCK_SIZE		(1 << BT_BLOCK_BITS)
#define BT_BLOCK_MASK		(BT_BLOCK_SIZE - 1)

/* fs block size */
#define FS_BLOCK_BITS		14
#define FS_BLOCK_SIZE		(1 << FS_BLOCK_BITS)
#define FS_BLOCK_MASK		(FS_BLOCK_SIZE - 1)

#define DEFAULT_MIN_PORT 6881
#define DEFAULT_MAX_PORT 6889

#define DEMAND_PRIORITY 7
#define PROFILE_CLOSE_PRIORITY 6
#define PROFILE_DISTANT_PRIORITY 5

#endif
