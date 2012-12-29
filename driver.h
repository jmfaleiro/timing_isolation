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

#endif
