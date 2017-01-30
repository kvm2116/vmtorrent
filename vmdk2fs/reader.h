#ifndef __READER_H__
#define __READER_H__

#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <string.h>
#include "vixDiskLib.h"

#define handle_error(msg) \
           do { perror(msg); exit(EXIT_FAILURE); } while (0)

#define MAGIC_OFFSET 32256 //63 sectors to skip, goes directly to partition
#define STRING_OFFSET 10   //STRING = mountpoint, directory named for vmware-mount

typedef struct SparseExtentHeader //the VMDK File Header
{
    uint32      magicNumber;
    uint32      version;
    uint32      flags;
    SectorType  capacity;
    SectorType  grainSize;
    SectorType  descriptorOffset;
    SectorType  descriptorSize;
    uint32      numGTEsPerGt;
    SectorType  rgdOffset;
    SectorType  gdOffset;
    SectorType  overHead;
    Bool        uncleanShutdown;
    char        singleEndLineChar;
    char        nonEndLineChar;
    char        doubleEndLineChar1;
    char        doubleEndLineChar2;
    uint16      compressAlgorithm;
    uint8       pad[433];

} __attribute__((__packed__)) SparseExtentHeader;

unsigned long long convertOffset(char* fileName, long long offset);
void lookupOffset(unsigned long long offset, sqlite3* db);
int buildCache(char* fileName);

#ifdef __DEBUG
void dumpHeader(SparseExtentHeader header);

static void DumpBytes(const unsigned char *buf,     // IN
                      size_t n,                     // IN
                      int step);
void verifyDump(char* fileName, uint32 beginningSector, uint32 diskOffset, uint32 noSector);
#endif


#endif
