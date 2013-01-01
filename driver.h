#ifndef DRIVER_HH
#define DRIVER_HH

#include <inttypes.h>
#include <pthread.h>
#include "commands.h"

#define HOPS 100
#define LINE_SIZE 64

struct list;

struct list {
  
  char *val;
  struct list * next;
};

typedef struct list List;


uint32_t*
genIndices(int numTests, int socket);

uint32_t
genNextIndex(int *acc, int numDone);

int
isValidIndex(int *acc, int numDone, int curr);

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
writeAnswers(int from, int to, uint64_t *results);

void
run(List *tests, List *randoms);

extern void*
runTest(void *arg);

extern void*
randomActivity(void *arg);

extern void*
alloc_mem(size_t size, int socket);


#endif
