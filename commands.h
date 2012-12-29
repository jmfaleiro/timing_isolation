#ifndef COMMANDS_HH
#define COMMANDS_HH

#include <inttypes.h>

typedef struct{

  int socket;
  uint64_t size;
} allocCommand;

typedef struct { 

  int from;
  int to;
  uint64_t retries;
} testCommand;

typedef struct {

  int socket;
  uint64_t size;
  char *buf;
} flushCommand;

#endif
