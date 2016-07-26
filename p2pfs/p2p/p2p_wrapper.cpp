#include "p2p_interface.hpp"
#include "p2p_wrapper.hpp"

P2P_EXTERN void *p2p_c_init(char *torrent, char *storage, char *peer_file, int demand, int upload_limit, int download_limit)
{
	p2p_interface *pi;
	int ret;

	pi = new p2p_interface();
	ret = pi->start_torrent_(torrent, storage, -1, -1, demand, upload_limit, download_limit);
	if (peer_file) {
	  pi->add_peer_file(peer_file); 
	} 
	return (void *) pi;
}

P2P_EXTERN void p2p_c_exit(void *p)
{
	p2p_interface *pi = (p2p_interface *) pi;
	delete pi;
}

P2P_EXTERN int p2p_c_getblock(void *p, int block, char *buffer, int bufsize)
{
	p2p_interface *pi = (p2p_interface *) p;
	return pi->get_block(block, &buffer, bufsize);
}

P2P_EXTERN void p2p_c_set_block_priority(void *p, int block_index, int priority)
{
	p2p_interface *pi = (p2p_interface *) p;
	int piece_index = block_index / pi->blocks_per_piece; 
	return pi->set_piece_priority(piece_index, priority);
}


P2P_EXTERN short p2p_c_register_push(void *p, void* (*func)(void *))
{
#ifdef VMTORRENT_PUSH
	p2p_interface *pi = (p2p_interface *) p;
	pi->p2p_push = func;
	//	P2P_OUT << "p2p_push set, p2p_push="<< func << std::endl;
	return 1;
#else
	return 0;
#endif
}

P2P_EXTERN int p2p_c_add_profile(void *p, char *profile){
	p2p_interface *pi = (p2p_interface *) p;
	return pi->add_profile(profile, PROFILE_CLOSE_PRIORITY);
}

P2P_EXTERN int p2p_c_blocks_per_piece(void *p){
	p2p_interface *pi = (p2p_interface *) p;
	return pi->blocks_per_piece;
}

P2P_EXTERN void p2p_c_set_diversity_window(void *p, int window){
	p2p_interface *pi = (p2p_interface *) p;
	pi->random_diversity_window = window;
}

P2P_EXTERN void p2p_c_set_bt_profile_window(void *p, int window){
	p2p_interface *pi = (p2p_interface *) p;
	pi->bt_profile_window = window;
}
