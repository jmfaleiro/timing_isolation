#ifndef DRIVER_HH
#define DRIVER_HH

#include <inttypes.h>
#include <pthread.h>
#include "commands.h"

struct list;

struct list {
  
  char *val;
  struct list * next;
};

typedef struct list List;

int
withinBoundary(uint32_t index, uint32_t val);

uint32_t*
genIndices(int numTests, int socket);

uint32_t
genNextIndex(uint32_t *acc, int numDone);

int
isValidIndex(uint32_t *acc, int numDone, uint32_t curr);

List* 
insert(List *start, char *val);

void
errorArg();

int
str2int(char *str);

int
parseNum(char **given_str);

int
listSize(List *start, int cur);

int
startRandoms(List *randoms, pthread_t **randomThreads, testCommand **randomCommands);

void
startTests(List *tests);

void
writeAnswers(int from, int to, long double *results);

void
run(List *tests, List *randoms);

extern void*
runTest(void *arg);

extern void*
randomActivity(void *arg);

extern void*
alloc_mem(size_t size, int socket);

extern void*
runTestSecond(void *arg);

#endif
