#ifndef COMMANDS_HH
#define COMMANDS_HH

#define CACHE_SIZE 24*(1<<20)
#define MEM_SIZE 1<<20
#define RANDOM_SIZE 24*(1<<29)
#define HALF_INTERVAL 3

#include <inttypes.h>

typedef struct{

  int socket;
  uint64_t size;
} allocCommand;

typedef struct { 

  int from;
  int to;
  uint64_t retries;
  uint32_t * randoms;
  int hops;
  
} testCommand;

typedef struct {

  int socket;
  uint64_t size;
  char *buf;
} flushCommand;

#endif
