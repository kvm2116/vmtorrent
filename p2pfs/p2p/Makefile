
libtorrent_dir = ../libtorrent
boost_install_dir = ../boost/install

CXXCOMPILE = $(CXX) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) \
	$(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CXXFLAGS) $(CXXFLAGS)
LTCXXCOMPILE = $(LIBTOOL) --tag=CXX $(AM_LIBTOOLFLAGS) $(LIBTOOLFLAGS) \
	--mode=compile $(CXX) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) \
	$(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CXXFLAGS) $(CXXFLAGS)
CXXLD = $(CXX)
CXXLINK = $(LIBTOOL) --tag=CXX $(AM_LIBTOOLFLAGS) $(LIBTOOLFLAGS) \
	--mode=link $(CXXLD) $(AM_CXXFLAGS) $(CXXFLAGS) $(AM_LDFLAGS) \
	$(LDFLAGS) -o $@
SOURCES = $(client_test_SOURCES)
DIST_SOURCES = $(client_test_SOURCES) 

CXXLINKSIMPLE = $(CXX) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) \
	$(CPPFLAGS) $(CXXFLAGS) $(LIB_PATH)

BOOST_ASIO_LIB = -lboost_system
BOOST_CPPFLAGS = -pthread -I$(boost_install_dir)/include
BOOST_FILESYSTEM_LIB = -lboost_filesystem
BOOST_LDFLAGS = -L$(boost_install_dir)/lib
BOOST_PROGRAM_OPTIONS_LIB = -lboost_program_options
BOOST_PYTHON_LIB = 
BOOST_REGEX_LIB = -lboost_regex
BOOST_SYSTEM_LIB = -lboost_system
BOOST_THREAD_LIB = -lboost_thread
LEVELDB_LIBS = -lleveldb -lsnappy -lpthread

#BOOST_FILESYSTEM_LIB = -lboost_filesystem-gcc41-1_34_1
#BOOST_LDFLAGS = -L$(boost_install_dir)/lib
#BOOST_PROGRAM_OPTIONS_LIB = -lboost_program_options-gcc41-1_34_1
#BOOST_PYTHON_LIB = 
#BOOST_REGEX_LIB = -lboost_regex-gcc41-1_34_1
#BOOST_SYSTEM_LIB = 
#BOOST_THREAD_LIB = -lboost_thread-gcc41-mt-1_34_1

libtorrent_lib = $(libtorrent_dir)/src/.libs
boost_lib = $(boost_install_dir)/lib
export LD_LIBRARY_PATH += $(CURDIR)/$(libtorrent_lib):$(CURDIR)/$(boost_lib)

LDFLAGS =  -L$(boost_lib) -L$(libtorrent_lib)
LIB_PATH = -L$(boost_lib) -L$(libtorrent_lib)

CXX = g++
DEFAULT_INCLUDES = -I. -I$(boost_install_dir)/include 
CXXFLAGS += -g -O2 -DTORRENT_STATS=1 -DTORRENT_DISK_STATS=1
#AM_CXXFLAGS = -ftemplate-depth-50 -I$(libtorrent_dir)/include -I$(libtorrent_dir)/include/libtorrent -Os -pthread -DBOOST_MULTI_INDEX_DISABLE_SERIALIZATION
AM_CXXFLAGS = -ftemplate-depth-50 -I$(libtorrent_dir)/include -I$(libtorrent_dir)/include/libtorrent -Os -pthread -DBOOST_MULTI_INDEX_DISABLE_SERIALIZATION
CPPFLAGS +=  -pthread -I$(boost_install_dir)/include/boost -DTORRENT_STATS=1 -DTORRENT_DISK_STATS=1

LIBTOOL = /bin/bash libtool
CXXLD = $(CXX)
AM_LDFLAGS = ${LDLAGS} -L./  -lboost_system -lboost_filesystem -lboost_thread  -DBOOST_MULTI_INDEX_DISABLE_SERIALIZATION -L/usr/lib -lssl -lcrypto
LIBS = -lz -lrt

client_test_SOURCES = client_test.cpp
#client_test_LDADD = $(libtorrent_dir)/src/libtorrent-rasterbar.la -lboost_system -lboost_regex -lboost_program_options
client_test_LDADD = -ltorrent-rasterbar -lboost_system -lboost_filesystem -lboost_regex -lboost_program_options

get_vm_LDADD = $(client_test_LDADD)


DEFS = -DSTDC_HEADERS=1 -DHAVE_SYS_TYPES_H=1 -DHAVE_SYS_STAT_H=1 -DHAVE_STDLIB_H=1 -DHAVE_STRING_H=1 -DHAVE_MEMORY_H=1 -DHAVE_STRINGS_H=1 -DHAVE_INTTYPES_H=1 -DHAVE_STDINT_H=1 -DHAVE_UNISTD_H=1 -DHAVE_DLFCN_H=1 -DLT_OBJDIR=\".libs/\" -D_FILE_OFFSET_BITS=64 -DHAVE_PTHREAD=1 -DHAVE___INCLUDE_LIBTORRENT_ASIO_HPP=1 -DHAVE___INCLUDE_LIBTORRENT_ASIO_SSL_STREAM_HPP=1 -DHAVE___INCLUDE_LIBTORRENT_ASIO_IP_TCP_HPP=1 -DHAVE_BOOST=/\*\*/ -DHAVE_BOOST_FILESYSTEM=/\*\*/ -DHAVE_BOOST_THREAD=/\*\*/ -DHAVE_BOOST_REGEX=/\*\*/ -DHAVE_BOOST_PROGRAM_OPTIONS=/\*\*/ -DBOOST_ASIO_HASH_MAP_BUCKETS=1021 -DTORRENT_USE_OPENSSL=/\*\*/ -DHAVE_LIBZ=1 -DWITH_SHIPPED_GEOIP_H=1 -DBOOST_EXCEPTION_DISABLE=/\*\*/ -DNDEBUG=/\*\*/

DEPDIR = .deps

P2P_OBJ = p2p_interface.o p2p_wrapper.o
P2P_DEP = p2p_interface.hpp vmtorrent_defines.hpp Makefile

#all: test_blaster test_p2p make_torrent read_allblocks profile_reader
all: test_p2p make_torrent

$(P2P_OBJ): $(P2P_DEP)

test_blaster: test_blaster.o $(P2P_OBJ)
	echo $(LD_LIBRARY_PATH)
	$(CXXLINKSIMPLE) -o $@ $^ $(get_vm_LDADD) $(LIBS)

test_p2p: test_p2p.o $(P2P_OBJ)
	$(CXXLINKSIMPLE) -o $@ $^ $(get_vm_LDADD) $(LIBS) $(LEVELDB_LIBS)
 
profile_reader: profile_reader.o $(P2P_OBJ)
	$(CXXLINKSIMPLE) -o $@ $^ $(get_vm_LDADD) $(LIBS)

make_torrent: make_torrent.o $(P2P_OBJ)
	$(CXXLINKSIMPLE) -o $@ $^ $(get_vm_LDADD) $(LIBS)

read_allblocks: read_allblocks.o $(P2P_OBJ)
	$(CXXLINKSIMPLE) -o $@ $^ $(get_vm_LDADD) $(LIBS)

.cpp.o:
	$(CXXCOMPILE)  -c -o $@ $<

clean:
	rm -rf *.o test_blaster test_p2p profile_reader make_torrent read_allblocks
