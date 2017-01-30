/**************************************************************/
/*                                                            */
/*                     VMDK2FS@CSCU                           */
/*        Copyright 2010 Peter Du. All rights reserved        */
/*                                                            */
/**************************************************************/

/*--------------------------------------------------------------
 *
 *  OUTLINE:
 *    This file intends to take in the absolute offset in the
 *    vmdk file and convert that into the offset seen by the
 *    guest operating system sees. And subsequently use the
 *    converted offset to look up in database to find out what
 *    file exactly it is and the length of the file
 *
 *  IMPLEMENTATION:
 *    It reads input from commandline, pass the arguments to
 *    the converter for converting the disk offset into guest
 *    offset. Then the guest offset is passed to a lookup
 *    function to look it up in the database
 *
 *  NOTES:
 *    Reference to the published white paper by VMWARE
 *    regarding the file header structure
 *
 *------------------------------------------------------------*/

#include "reader.h"

//Grain Table Cache
uint32* cacheTable=NULL;

int main(int argc, char* argv[])
{
    if (argc <= 3)
    {
        fprintf(stderr, "Usage: %s <vmdk name> <host offset> <database name> \n", argv[0]);
        return -1;
    }

//
// Step 1. Read Meta Information from vmdk file and build the cache


// Step 2. Open the database for query

// Step 3. Enter into a giant look for query

//
    long long offset = atoll(argv[2]);
    unsigned long long hostoffset = convertOffset(argv[1], offset);

    sqlite3* db;
    int rc;

    if (hostoffset) {
		hostoffset =  hostoffset - MAGIC_OFFSET;
#ifdef __DEBUG
		printf("reading %llu from database %s\n", hostoffset, argv[3]);
#endif
        sqlite3_initialize();

        rc = sqlite3_open(argv[3], &db);

        if (rc)
        {
            fprintf(stderr, "Can't open database %s\n", sqlite3_errmsg(db));
            sqlite3_close(db);
            exit(1);
        }

        lookupOffset(hostoffset, db);
	}

	if (db)
	{
	    sqlite3_close(db);
	    sqlite3_shutdown();
	}

    return 0;
}

int buildCache(char *fileName)
{
	return 0; //success
}

unsigned long long convertOffset(char* fileName, long long offset)
{


    FILE* vmdisk;
    vmdisk = fopen(fileName, "r");
    if (!vmdisk)
        handle_error("Opening file");


    SparseExtentHeader header;
    int r;

    r = fread(&header, sizeof header, 1, vmdisk);
    if (r!= 1)
        handle_error("Reading file");

#ifdef __DEBUG
    dumpHeader(header);
#endif

    //Sector of File: the offset represented in sectors, floored.
    unsigned long long sectorOfFile = offset / VIXDISKLIB_SECTOR_SIZE;

#ifdef __DEBUG
    printf("sectorOfFile:\t\t\t%llu\n", sectorOfFile);
#endif

    if (sectorOfFile >= header.overHead)
    {
        printf("Action:\t\t\treading from disk\n");
    }
    else
    {
        printf("Action:\t\t\treading meta data\n");
        //TODO:Meta data reading;
        return 0; //the actual offset is nay
    }

    //
    unsigned long long grainSector = ((sectorOfFile - header.overHead) / header.grainSize) * header.grainSize + header.overHead;
    unsigned grainOffset = offset - grainSector * VIXDISKLIB_SECTOR_SIZE;

#ifdef __DEBUG
    printf("grainSector:\t\t\t%llu\n", grainSector);
#endif

    //Iterate over the grains
    unsigned long grainCount = header.capacity / header.grainSize;
    printf("Number of grains:\t\t%lu\n", grainCount);

    //Read the first entry in the grain directory
    //to get the offset of first grain table entry ;
    if (fseek(vmdisk, header.gdOffset * VIXDISKLIB_SECTOR_SIZE, SEEK_SET)!= 0)
        handle_error("Cannot set position for firstGTEOffset");

    uint32 firstGTEOffset;
    r = fread(&firstGTEOffset, sizeof firstGTEOffset, 1, vmdisk);
    if (r!=1)
        handle_error("Reading firstGTEOffset");
    printf("First GTE Offset:\t\t%u(Bytes:%u)\n", firstGTEOffset, firstGTEOffset * VIXDISKLIB_SECTOR_SIZE);

    //Get all GTE
    uint32 i;
    uint32 allocatedCount;
    allocatedCount = 0;

    uint32 hostSectorOffset;
    uint32 diskStartingOffset;
    unsigned long long hostOffset;

	uint32 grainTableEntry;
	//read the entries into cache
	if (cacheTable == NULL)
	{
		cacheTable = (uint32*) malloc(grainCount * sizeof(uint32));

    	for (i=0; i <grainCount; i++)
		{
        	if (fseek(vmdisk, firstGTEOffset * VIXDISKLIB_SECTOR_SIZE + i * (sizeof i), SEEK_SET)!= 0)
            	handle_error("Cannot set position for GTE");

			r = fread(&grainTableEntry, sizeof grainTableEntry, 1, vmdisk);

			if (r!=1)
				handle_error("Reading grainTableEntry");

#ifdef __DEBUG
			if (grainTableEntry!=0)
				printf("%u Grain Table Entry:\t%u(Bytes:%u)\n", i, grainTableEntry, grainTableEntry * VIXDISKLIB_SECTOR_SIZE);
#endif
			cacheTable[i] = grainTableEntry;
		}
	}

    for (i=0; i <grainCount; i++)
    {
        if (grainSector == cacheTable[i])
        {
            hostOffset = i * header.grainSize * VIXDISKLIB_SECTOR_SIZE + grainOffset;
            printf("Reading Grain:\t\t%u\nSector Offset:\t\t%u\n", i, grainOffset);
            printf("Host Offset:\t\t%llu\n", hostOffset);
            printf("Disk Offset:\t\t%llu\n", offset);

            hostSectorOffset = i*header.grainSize + grainOffset / VIXDISKLIB_SECTOR_SIZE;
            diskStartingOffset = offset / VIXDISKLIB_SECTOR_SIZE * VIXDISKLIB_SECTOR_SIZE ;
            break;
        }
    }

    fclose(vmdisk);

#ifdef __DEBUG
    verifyDump(fileName, hostSectorOffset, diskStartingOffset, 1);
#endif
    return hostOffset;
}

void lookupOffset(unsigned long long offset, sqlite3* db)
{

    char* sql;
    int rc;
    sqlite3_stmt *stmt = NULL;
    sql = "select name, length from files where start <=:offset order by start desc limit 1 ";
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK)
        exit(-1);
    sqlite3_bind_int64(stmt, sqlite3_bind_parameter_index(stmt, ":offset"), offset);
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        printf("Reading file:\t\t%s\n",(sqlite3_column_text(stmt, 0)+STRING_OFFSET));
        printf("File Length:\t\t%s\n",sqlite3_column_text(stmt, 1));
    }

    sqlite3_finalize(stmt);

}

#ifdef __DEBUG
void verifyDump(char* fileName, uint32 beginningSector, uint32 diskStartingOffset, uint32 noSector)
{
    VixError vixError;

    vixError = VixDiskLib_Init(1,0,NULL, NULL, NULL, "/usr/lib/vmware-vix-disklib/plugins32/");
    if (VIX_FAILED(vixError))
    {
        fprintf(stderr, "Can't initialize Disk Library");
        exit(EXIT_FAILURE);
    }

    VixDiskLibConnectParams cnxParams = {0};
    VixDiskLibConnection connection;
    vixError = VixDiskLib_Connect(&cnxParams, &connection);

    if (VIX_FAILED(vixError))
    {
        fprintf(stderr, "Can't create connection");
        exit(EXIT_FAILURE);
    }

    VixDiskLibHandle handle;

    vixError = VixDiskLib_Open(connection, fileName, VIXDISKLIB_FLAG_OPEN_READ_ONLY, &handle);
    if (VIX_FAILED(vixError))
    {
        fprintf(stderr, "Can't open file");
        exit(EXIT_FAILURE);
    }

    uint8 buf[VIXDISKLIB_SECTOR_SIZE];
    VixDiskLibSectorType i;
    printf("Library Dump:\n");
    for (i = 0; i < noSector; i++)
    {
        vixError = VixDiskLib_Read(handle, beginningSector+i,1, buf);
        if (VIX_FAILED(vixError))
        {
            fprintf(stderr, "Can't read");
            exit(EXIT_FAILURE);
        }
        DumpBytes(buf, sizeof buf, 16);

    }

    if (handle)
    {
        VixDiskLib_Close(handle);
    }
    printf("Disk Dump:\n");

    FILE* vmdisk = fopen(fileName, "r");
//    printf("disk offset:%u\n", diskStartingOffset);
    fseek(vmdisk, diskStartingOffset, SEEK_SET);
    printf("current pos:%li\n", ftell(vmdisk));
    uint8 buffer[VIXDISKLIB_SECTOR_SIZE];
    fread(buffer, sizeof buffer, 1, vmdisk);
    DumpBytes(buffer, sizeof buffer, 16);
    fclose(vmdisk);
}


void dumpHeader(SparseExtentHeader header)
{
    printf("\nMagic Number:\t\t\t0x%x\nVersion:\t\t\t%u\nFlags:\t\t\t\t0x%x\nCapacity:\t\t\t%llu sectors\nGrain Size:\t\t\t%llu sectors\nDescriptor Offset:\t\t%llu sector(s)\nDescriptor Size:\t\t%llu sector(s)\n",
           header.magicNumber, header.version, header.flags, header.capacity, header.grainSize, header.descriptorOffset, header.descriptorSize);
    printf("Grain Table Entries per table:\t%u\nRedundant Grain Directory:\t%llu sectors\nGrain Directory:\t\t%llu sectors\nOverhead:\t\t\t%llu sectors\nUnclear shutdown:\t\t%i\n",
           header.numGTEsPerGt, header.rgdOffset, header.gdOffset, header.overHead, header.uncleanShutdown);
    printf("singleEndLineChar:\t\t%i\nnonEndLineChar:\t\t\t%i\ndoubleEndLineChar1:\t\t%i\ndoubleEndLineChar2:\t\t%i\nCompression:\t\t\t%u\n",
           header.singleEndLineChar, header.nonEndLineChar, header.doubleEndLineChar1, header.doubleEndLineChar2, header.compressAlgorithm);
}

/*
 *----------------------------------------------------------------------
 *
 * DumpBytes --
 *
 *      Displays an array of n bytes.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */

static void
DumpBytes(const unsigned char *buf,     // IN
          size_t n,                     // IN
          int step)                     // IN
{
    size_t lines = n / step;
    size_t i;

    for (i = 0; i < lines; i++)
    {
        int k, last;
        printf("%04"FMTSZ"x : ", i * step);
        for (k = 0; n != 0 && k < step; k++, n--)
        {
            printf("%02x ", buf[i * step + k]);
        }
        printf("  ");
        last = k;
        while (k --)
        {
            unsigned char c = buf[i * step + last - k - 1];
            if (c < ' ' || c >= 127)
            {
                c = '.';
            }
            printf("%c", c);
        }
        printf("\n");
    }
    printf("\n");
}
#endif
