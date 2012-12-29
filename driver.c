#include <unistd.h>
#include <pthread.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "driver.h"
#include "commands.h"

int retries;

List * insert(List *start, char *val)
{
  List *temp = (List *)malloc(sizeof(List));
  temp->next = start;
  temp->val = val;
  return temp;
}

void errorArg()
{
  fprintf(stderr, "Got unexpected option %s\n", optarg);
  fprintf(stderr, "Three options allowed:\n-t from-to => from = socket on which to run thread, to = socket on which to allocate mem\n");
  fprintf(stderr, "-r from-to => same as above, but used for generating traffic, not testing\n");
  fprintf(stderr, "-n num_tests => the number of iterations for the test (specified ***EXACTLY*** once)\n");
  exit(0);
}

int
str2int(char *str)
{
  int ret = 0;
  while(*str != '\0')
    ret = 10*ret + (*(str++) - '0');

  return ret;
}

int main(int argc, char **argv)
{
  retries = -1;
  const char *optString = "t:r:n:";
  List* tests = NULL;
  List* randoms = NULL;
  int opt;
  
  while((opt = getopt(argc, argv, optString)) != -1){
    
    switch(opt) {
      
    case 't':
      tests = insert(tests, optarg);
      break;
      
    case 'r':
      randoms = insert(randoms, optarg);
      break;

    case 'n':
      if (retries >= 0)
	errorArg();
      retries = str2int(optarg);
      break;
      
    case '?':
      errorArg();
      break;
      
    default:
      break;
    }    
  }

  if(retries < 0)
    errorArg();
  
  run(tests, randoms);
}

int
parseNum(char **given_str)
{
  int i = 0;
  char *str = *given_str;
  int running = 0;
  
  while(str[i] >= '0' && str[i] <= '9'){
    running = 10*running + (str[i] - '0');
    ++i;
  }

  *given_str = str+i;
  return running;      
}

int
listSize(List *start, int cur)
{
  if (start == NULL)
    return cur;
  else
    return listSize(start->next, cur+1);
}

int
startRandoms(List *randoms, pthread_t **randomThreads, testCommand **randomCommands)
{
  int i = 0;
  int randomSize = listSize(randoms, 0);
  
  *randomThreads = (pthread_t *)malloc(sizeof(pthread_t) * randomSize);
  *randomCommands = (testCommand *)malloc(sizeof(testCommand) * randomSize);  

  while(randoms){
    
    // Parse out the socket numbers
    char *str = randoms->val;
    int from = parseNum(&str);
    str = str+1;
    int to = parseNum(&str);
    
    // set-up the commands
    (*randomCommands)[i].to = to;
    (*randomCommands)[i].from = from;
    
    // Go!
    pthread_create((*randomThreads)+i, NULL, randomActivity, (void *) ((*randomCommands)+i));    
    
    randoms = randoms->next;
    ++i;
  }

  return randomSize;
}

void
startTests(List *tests)
{
  int i = 0;
  int testsSize = listSize(tests, 0);
  uint64_t ** results = (uint64_t **)malloc(sizeof(uint64_t)*testsSize);

  pthread_t *testThreads = (pthread_t *)malloc(sizeof(pthread_t) * testsSize);
  testCommand *testCommands = (testCommand *)malloc(sizeof(testCommand) * testsSize);

  while(tests){
    
    // Parse out the socket numbers
    char *str = tests->val;    
    int from = parseNum(&str);
    str = str+1;
    int to = parseNum(&str);
    
    // set-up the commands
    testCommands[i].to = to;
    testCommands[i].from = from;
    testCommands[i].retries = retries;
    
    // Go!
    pthread_create(testThreads+i, NULL, runTest, (void *) (testCommands+i));    
    
    tests = tests->next;
    ++i;
  }
  
  for(i = 0; i < testsSize; ++i)
    pthread_join(testThreads[i], (void **)(results+i));
  
  for(i = 0; i < testsSize; ++i)
    writeAnswers(testCommands[i].from, 
		 testCommands[i].to,
		 results[i]);
}


void
writeAnswers(int from, int to, uint64_t *results)
{
  FILE *fp; 
  char filename[30];
  sprintf(filename, "from-%d-to-%d.txt", from, to);

  if((fp = fopen(filename, "w")) == NULL)
    fprintf(stderr, "Error while opening output file \n%s\n", strerror(errno));
 
  int i, j;
  fprintf(fp, "From %d, to %d\n", from, to);
    
  for(i=0; i<retries; ++i)    
    fprintf(fp, "%"PRIu64"\n", results[i]);
    
  fprintf(fp, "\n\n");  
  fclose(fp);          
}

void
run(List *tests, List *randoms)
{
  pthread_t *randomThreads;
  testCommand* randomCommands;
  
  int randomSize = startRandoms(randoms, &randomThreads, &randomCommands);
  startTests(tests);
  
  int i;
  for(i = 0; i < randomSize; ++i)
    pthread_cancel(randomThreads[i]);    
}