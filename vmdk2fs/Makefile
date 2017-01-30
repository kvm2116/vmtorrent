CC=gcc
CFLAGS=-Wall

all: c2db reader

c2db: c2db.c
	$(CC) $(CFLAGS) c2db.c -lsqlite3 -o ../demo/c2db

reader: reader.c
	$(CC) $(CFLAGS) reader.c -I/usr/lib/vmware-vix-disklib/include/ -lsqlite3 -o ../demo/reader

readerdebug: reader.c
	$(CC) $(CFLAGS) -lsqlite3 -I/usr/lib/vmware-vix-disklib/include/ -D__DEBUG -o ../demo/reader reader.c  /usr/lib/vmware-vix-disklib/lib32/libvixDiskLib.so 

fiemap: fiemap.c
	$(CC) $(CFLAGS) fiemap.c -o ../demo/fiemap

fibmap: fibmap.c
	$(CC) $(CFLAGS) fibmap.c -o ../demo/fibmap
