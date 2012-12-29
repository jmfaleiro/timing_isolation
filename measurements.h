#ifndef MEASUREMENTS_H
#define MEASUREMENTS_H

#define CACHE_SIZE 1000
#define MEM_SIZE 1000
#define RANDOM_SIZE 1000

#include <inttypes.h>

void
serialize();

void* 
alloc_mem(size_t size, int socket);

uint32_t* 
genRandoms(int socket, int retries);

void*
flushCache(void *arg);

void
flushCacheInner(uint64_t size, int socket, char *ptr);

uint64_t
readStart();

uint64_t
readEnd();

flushCommand
initFlushCommand(int socket, uint64_t size);

uint64_t
latencySingle(char volatile *ptr, char *set);

void*
randomActivity(void *arg);

void
pinThread(int socket);

void* 
runTest(void *arg);


#endif
