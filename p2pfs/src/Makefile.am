## Process this file with automake to produce Makefile.in

bin_PROGRAMS = bindfs

noinst_HEADERS = debug.h permchain.h userinfo.h misc.h usermap.h rate_limiter.h bindfs.h
bindfs_SOURCES = bindfs.c debug.c permchain.c userinfo.c misc.c usermap.c rate_limiter.c blocks.c

AM_CPPFLAGS = ${my_CPPFLAGS} ${fuse_CFLAGS}
AM_CFLAGS = ${my_CFLAGS}

export LD_LIBRARY_PATH += $(CURDIR)/../libtorrent/src/.libs:$(CURDIR)/../boost/install/lib

p2p_LIBS = ../p2p/p2p_wrapper.o ../p2p/p2p_interface.o \
		-L../boost/install/lib -L../libtorrent/src/.libs \
		-ltorrent-rasterbar -lboost_regex -lboost_program_options -lz -lrt

bindfs_LDADD = $(p2p_LIBS) $(fuse_LIBS) ${my_LDFLAGS}

man_MANS = bindfs.1
