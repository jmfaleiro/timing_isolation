#ifndef MEASUREMENTS_H
#define MEASUREMENTS_H

#include <sys/mman.h>

#ifndef MAP_HUGETLB
#define MAP_HUGETLB 0x40000
#endif

#define MMAP_FLAGS (MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB | MAP_FIXED)
#define MMAP_PROT (PROT_READ | PROT_WRITE)
#define MMAP_ADDR (void *)(0x8000000000000000UL)

#include <inttypes.h>

void*
alloc_huge_pages(size_t size, int socket);

void*
alloc_huge_pages_inner(void* huge_cmd);

void
unalloc_huge_pages(void* addr, size_t size);

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

extern int
flush;


#endif
