#ifndef __READERLIB_H
#define __READERLIB_H

int 
initializeWithFiles (char* vmdkFile, char* sqliteFile);

int 
buildGrainEntryCacheTable (uint32_t** cacheTable);

uint64_t 
convertOffsetDiskToGuest (uint64_t diskOffset);

void
lookupGuestOffset (uint64_t guestOffset);

#endif
