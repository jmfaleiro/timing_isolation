#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <numa.h>
#include <inttypes.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <numaif.h>

#include "commands.h"
#include "measurements.h"

#define MAXNODES 8

static inline void serialize() __attribute__((always_inline));
static inline uint64_t readStart() __attribute__((always_inline));
static inline uint64_t readEnd() __attribute__((always_inline));


static inline void serialize () 
{
  asm volatile("cpuid\n\t":::
	       "%rax", "%rbx", "%rcx", "%rdx");	       
}

// Wrapper to allocate a huge page.
// Don't want to do a membind initially because
// children will be constrained by the same
// policy. We do a membind only when we are
// at "leaf" children.

void*
alloc_huge_pages(size_t size, int socket)
{
  pthread_t thread;
  void *addr;
  
  allocCommand cmd;
  cmd.size = size;
  cmd.socket = socket;

  pthread_create(&thread, NULL, alloc_huge_pages_inner, (void *) &cmd);
  pthread_join(thread, (void **) &addr);
  
  return addr;
}


// This is the actual huge_page allocation function.

void*
alloc_huge_pages_inner(void * huge_cmd)
{
  // Make sure we're executing and allocating on the 
  // socket we want.

  int socket = ((allocCommand *)huge_cmd)->socket;
  size_t size = ((allocCommand *)huge_cmd)->size;
  
  char digits[10];
  sprintf(digits, "%d", socket);
  

  pinThread(socket);
  numa_set_strict(1);
  
  void *ret = mmap(MMAP_ADDR, size, MMAP_PROT, MMAP_FLAGS, 0, 0);

  if (ret == MAP_FAILED){
    
    fprintf(stderr, "Couldn't allocate a huge page: %s\n", strerror(errno));
    exit(0);
  }
  
  
  unsigned long mask[MAXNODES] = {0};
  mask[socket] = 1;
  /*
  if (mbind(ret, size, MPOL_BIND, mask, MAXNODES, 0) < 0){
    
    fprintf(stderr, "mbind failed: %s\n", strerror(errno));
    exit(0);
  }
  */
  int upper = size / sizeof(uint32_t);
  uint32_t* init = (uint32_t *)ret;
  int i;

  for(i = 0; i < upper; ++i)
    init[i] = i;

  return ret;  
}

void
unalloc_huge_pages(void *addr, size_t size)
{
  munmap(addr, size);
}

void*
alloc_mem(size_t size, int socket)
{
  numa_set_strict(1);
  char * buf = numa_alloc_onnode(size, socket);
  int temp = 0;
  
  if(buf == NULL){    
    fprintf(stderr, "Coudln't allocate memory on node %d\n", socket);  
    fprintf(stderr, "%s\n", strerror(errno));
    exit(0);
  }

  if ((temp = mlock(buf, size)) < 0){
    
    fprintf(stderr, "mlock couldn't lock memory to the CPU %d\n", socket);
    fprintf(stderr, "%s\n",strerror(errno));
    exit(0);
  }

  int i;
  for(i = 0; i < size; ++i)
    buf[i] = 0;
  
  return buf;      
}

uint32_t*
genRandoms(int socket, int retries)
{
  int i;  
  uint32_t *randoms = (uint32_t *)alloc_mem(sizeof(uint32_t) * retries, socket);

  for(i = 0; i < retries; ++i){
    
    randoms[i] = rand();
  }

  return randoms;
}


// Flush cache
void*
flushCache(void *arg)
{  
  flushCommand * flushArg = (flushCommand*)arg;

  uint64_t size = flushArg->size;
  int socket = flushArg->socket;
  char *ptr = flushArg->buf;
  
  flushCacheInner(size, socket, ptr);
  
  return NULL;
}

void flushCacheInner(uint64_t size, int socket, char volatile *ptr)
{
  pinThread(socket);
  serialize();

  uint64_t i;
  for(i = 0; i < size; i++)
    ptr[i]++;  
  
  serialize();
}

static inline uint64_t readStart()
{
  uint32_t cyclesHigh, cyclesLow;

  
  asm volatile("cpuid\n\t"
	       "rdtsc\n\t"
	       "movl %%edx, %0\n\t"
	       "movl %%eax, %1\n\t"
	       : "=r" (cyclesHigh), "=r" (cyclesLow) ::
		 "%rax", "%rbx", "%rcx", "%rdx");
  
  return (((uint64_t)cyclesHigh<<32) | cyclesLow);
}

static inline uint64_t readEnd()
{
  uint32_t cyclesHigh, cyclesLow;
  
  asm volatile("rdtscp\n\t"
	       "movl %%edx, %0\n\t"
	       "movl %%eax, %1\n\t"		 
	       "cpuid\n\t"		 
	       : "=r" (cyclesHigh), "=r" (cyclesLow) ::
		 "%rax", "%rbx", "%rcx", "%rdx");
  
  return (((uint64_t)cyclesHigh<<32) | cyclesLow);
}


flushCommand initFlushCommand(int socket, uint64_t size)
{
  flushCommand ret;  

  char *flushBuf = (char *)alloc_mem(size, socket);
  
  ret.socket = socket;
  ret.size = size;
  ret.buf = flushBuf;
  
  return ret;
}

long double
latencySingle(char volatile *ptr, uint32_t *indices, int size, int volatile *set)
{
  uint64_t start, end;
  volatile int blah;
  int i;
  
  blah = ptr[0];
  
  start = readStart();
  for(i = 0; i < size; ++i)    
    blah += ptr[1+i];
  end = readEnd();

  *set = blah;
  return (end - start) / ((long double)size);
}


void * randomActivity(void *arg)
{
  numa_set_strict(1);
  
  int from = ((testCommand *)arg)->from;
  int to = ((testCommand *)arg)->to;
  uint64_t volatile sum = 0;
  
  pinThread(from);

  char *mem = (char *)alloc_mem(RANDOM_SIZE, to);

  size_t i, j;

  while(1)
    for(i = 0; i < CACHE_SIZE; ++i)
      for(j = i; j < MEM_SIZE; j += CACHE_SIZE)      
	sum += mem[j];
  
  return NULL;
}

void pinThread(int socket)
{
  if (numa_run_on_node(socket) < 0){
    
    printf("Unable to pin thread properly\n");
    exit(0);
  }
}

void
setupMem(void *mem, uint32_t *indices, int size)
{

  int i;
  char *base = (char *)mem;
  *((char **)mem) = base + indices[0];
  
  char **start = (char **)mem;
  for(i = 0; i < size; ++i){
    
    char **temp = (char **)(*start);
    *temp = base + indices[1+i];
    start = temp;
  }  
}

long double
latencyHopping(char volatile**mem, char volatile **dummy)
{
  uint64_t start, end;
  char volatile ** begin = (char volatile **) (*mem);
  
  start = readStart();
  THO(begin)  
  end = readEnd();

  *dummy = *begin;  
  long double ret = ((long double)(end - start)) / ((long double) HOPS);
  return ret;
}

void*
runTestSecond(void *arg)
{
  numa_set_strict(1);

  int from = ((testCommand *)arg)->from;
  int to = ((testCommand *)arg)->to;
  int retries = ((testCommand *)arg)->retries;  
  uint32_t* randoms = ((testCommand *)arg)->randoms;
  int hops = ((testCommand *)arg)->hops;

  pinThread(from);

  char *mem = (char *)alloc_huge_pages(MEM_SIZE, to);

  flushCommand toFlushCommand;
  flushCommand fromFlushCommand;

  pthread_t fromThread;
  pthread_t toThread;

  if (flush){    

    if (from != to)    
      toFlushCommand = initFlushCommand(to, CACHE_SIZE);
    fromFlushCommand = initFlushCommand(from, CACHE_SIZE);
  }


  long double *results = (long double *)alloc_mem(sizeof(long double) * retries, from);
  setupMem(mem, randoms, hops);

  int i;
  for(i = 0; i < retries; ++i){

    // First flush the cache.
    if (flush){    
  
      // Flush the guy we're reading from.
      if(from != to)
	pthread_create(&toThread, NULL, flushCache, (void *)(&toFlushCommand));    
      
      pthread_create(&fromThread, NULL, flushCache, (void *)(&fromFlushCommand));
      
      pthread_join(toThread, NULL);
      pthread_join(fromThread, NULL);
    }
    
    char *temp;
    results[i] = latencyHopping((char volatile **)mem, (char volatile **) &temp);
  }

  unalloc_huge_pages(mem, MEM_SIZE);
  return results;
}


void * runTest(void *arg)
{
  numa_set_strict(1);

  int from = ((testCommand *)arg)->from;
  int to = ((testCommand *)arg)->to;
  int retries = ((testCommand *)arg)->retries;  
  uint32_t* randoms = ((testCommand *)arg)->randoms;
  int hops = ((testCommand *)arg)->hops;

  pinThread(from);

  char *mem = (char *)alloc_huge_pages(MEM_SIZE, to);

  flushCommand toFlushCommand;
  flushCommand fromFlushCommand;

  if (flush){    

    if (from != to)    
      toFlushCommand = initFlushCommand(to, CACHE_SIZE);
    fromFlushCommand = initFlushCommand(from, CACHE_SIZE);
  }

  long double *results = (long double *)alloc_mem(sizeof(long double) * retries, from);
  
  pthread_t toThread;

  int i;
  for(i = 0; i < retries; ++i){
    
    // First flush both caches, we want to do memory reads
    if (flush){
      
      if(from != to)
	pthread_create(&toThread, NULL, flushCache, (void *)(&toFlushCommand));    

      flushCacheInner(fromFlushCommand.size, fromFlushCommand.socket, fromFlushCommand.buf);
    }
    
    if(from != to)
      pthread_join(toThread, NULL);
    
    int temp;
    int index = i*hops;
    results[i] = latencySingle(mem, randoms+index, hops, &temp);
  }

  unalloc_huge_pages(mem, MEM_SIZE);  
  return results;
}






