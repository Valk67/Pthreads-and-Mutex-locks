

#ifndef MULTI_LOOKUP_H
#define MULTI_LOOKUP_H

#include "ut.h"
#include <sys/wait.h>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

//from lookup.c
#define USAGE "<inputFilePath> <outputFilePath>"
#define INPUTFS "%1024s"
//limits described in write up
#define MAX_INPUT_FILES 10 
#define MAX_RESOLVER_THREADS 10
#define MIN_RESOLVER_THREADS 2
#define MAX_NAME_LENGTH 1025
#define MIN_IP_LENGTH INET6_ADDRSTRLEN

//stuct manager holds the buff/mutex and other related items in shared
//memory 
struct Manager { 
  int count;
  
  FILE* output_f;
  pthread_mutex_t qLock; 
  pthread_mutex_t fLock;
  pthread_cond_t fill;
  pthread_cond_t empty;
  char boundedBuf[10][MAX_NAME_LENGTH];
  int done;

};
//function declarations.
void requester(char* input, struct Manager* m);
void resolver(struct Manager* m);
void segFinder(char * x);


#endif 
