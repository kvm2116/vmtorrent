#include "vmtorrent_defines.hpp"

/* interface for C */

#ifdef __cplusplus
#define P2P_EXTERN  extern "C"
#else
#define P2P_EXTERN  extern
#endif

P2P_EXTERN void *p2p_c_init(char *torrent, char *storage, char*seed, int demand, int upload_limit, int download_limit);
P2P_EXTERN void p2p_c_exit(void *p);
P2P_EXTERN int p2p_c_getblock(void *p, int block, char *buffer, int bufsize);
P2P_EXTERN void p2p_c_set_block_priority(void *p, int block_index, int priority);
P2P_EXTERN short p2p_c_register_push(void *p, void *(*func)(void *));
P2P_EXTERN int p2p_c_add_profile(void *p, char *profile);    
P2P_EXTERN int p2p_c_blocks_per_piece(void *p);    
P2P_EXTERN void p2p_c_set_bt_profile_window(void *p, int window);
P2P_EXTERN void p2p_c_set_diversity_window(void *p, int window);
