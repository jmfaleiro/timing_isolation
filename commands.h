#ifndef COMMANDS_HH
#define COMMANDS_HH


#define HOPS 1000
#define LINE_SIZE 64

#define CACHE_SIZE (24*(1<<21))
#define MEM_SIZE (1<<21)
#define INDEX_MASK (MEM_SIZE - 1)
#define RANDOM_SIZE (24*(1<<26))
#define HALF_INTERVAL 3

#define ONE(x) {x = (char volatile**)(*x);}
#define TEN(x) ONE(x) ONE(x) ONE(x) ONE(x) ONE(x) ONE(x) ONE(x) ONE(x) ONE(x) ONE(x)
#define HUN(x) TEN(x) TEN(x) TEN(x) TEN(x) TEN(x) TEN(x) TEN(x) TEN(x) TEN(x) TEN(x)
#define THO(x) HUN(x) HUN(x) HUN(x) HUN(x) HUN(x) HUN(x) HUN(x) HUN(x) HUN(x) HUN(x)

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
