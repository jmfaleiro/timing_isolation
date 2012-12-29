#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <numa.h>
#include <inttypes.h>

#include "commands.h"
#include "measurements.h"

inline void serialize()
{
  asm volatile("cpuid\n\t":::
	       "%rax", "%rbx", "%rcx", "%rdx");	       
}

void*
alloc_mem(size_t size, int socket)
{
  numa_set_strict(1);
  numa_alloc_onnode(size, socket);
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

void flushCacheInner(uint64_t size, int socket, char *ptr)
{
  pinThread(socket);
  serialize();

  uint64_t i;
  for(i = 0; i < size; i++)
    ptr[i]++;  
  
  serialize();
}

inline uint64_t readStart()
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

inline uint64_t readEnd()
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

uint64_t latencySingle(char volatile *ptr, char *set)
{
  uint64_t start, end;
  char blah;

  start = readStart();
  blah = *ptr;
  end = readEnd();

  *set = blah;
  return end - start;  
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

void * runTest(void *arg)
{
  numa_set_strict(1);

  int from = ((testCommand *)arg)->from;
  int to = ((testCommand *)arg)->to;
  int retries = ((testCommand *)arg)->retries;  

  pinThread(from);

  uint32_t *randoms = genRandoms(from, retries);
  char *mem = (char *)alloc_mem(MEM_SIZE, to);

  flushCommand toFlushCommand;  
  if (from != to)    
    toFlushCommand = initFlushCommand(to, CACHE_SIZE);

  flushCommand fromFlushCommand = initFlushCommand(from, CACHE_SIZE);

  uint64_t *results = (uint64_t *)alloc_mem(sizeof(uint64_t) * retries, from);
  
  pthread_t toThread, fromThread;

  int i;
  for(i = 0; i < retries; ++i){
    
    // First flush both caches, we want to do memory reads
    if(from != to)
      pthread_create(&toThread, NULL, flushCache, (void *)(&toFlushCommand));
    
    flushCacheInner(fromFlushCommand.size, fromFlushCommand.socket, fromFlushCommand.buf);
    
    if(from != to)
      pthread_join(toThread, NULL);
    
    uint32_t index = randoms[i] % MEM_SIZE;
    char temp;
    results[i] = latencySingle(mem+index, &temp);    
  }
  
  return results;
}






