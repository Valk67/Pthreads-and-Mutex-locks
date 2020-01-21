
#include "multi-lookup.h"

//4 functions below pretain to the stack and used throughout resovler and 
//requester.
//They behave like similar functions defined in c library queue.h but changed
//to accpet a struct pointer.
void push(struct Manager *m, char * x) {
  strncpy(m->boundedBuf[m->count], x, MAX_NAME_LENGTH);
  m->count++; 
}
void pop(struct Manager *m, char *item) {
  strncpy(item, m->boundedBuf[(m->count) - 1], MAX_NAME_LENGTH);
  m->count--;
}
int stack_empty(struct Manager *m) {
  if (m->count == 0) {
    return 1;
  }
  return 0;
}
int stack_full(struct Manager *m) { //made stack a max size of 10.
  if (m->count == 10) {
    return 1;
  }
  return 0;
}


/***************MAIN FUNCTION*******************/

int main(int argc, char* argv[]) {
  int domain_names = argc - 2;
  int system_cores = sysconf(_SC_NPROCESSORS_ONLN);
  int i = 0;
  
  //needed for limits
  if (argc < 3) { 
    fprintf(stderr, "Not enough arguments: %d\n", (argc - 1));
	  fprintf(stderr, "Usage:\n %s %s\n", argv[0], USAGE);
	  return EXIT_FAILURE;
  }
  if (system_cores < 2) {
    fprintf(stderr, "Too few resolver threads: %d\n", system_cores);
    return EXIT_FAILURE;
  }
  
  //initalization for stuct manager into shared memory
  struct Manager* manager = (struct Manager *) mmap(NULL, sizeof(struct Manager),
        PROT_READ | PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);

  //mutex initalization to shared memory
  pthread_mutexattr_t mutexAttr;
  pthread_mutexattr_setpshared(&mutexAttr, PTHREAD_PROCESS_SHARED);
  pthread_mutex_init(&manager->fLock, &mutexAttr); 
  pthread_mutexattr_t mutexAttr2;
  pthread_mutexattr_setpshared(&mutexAttr2, PTHREAD_PROCESS_SHARED);
  pthread_mutex_init(&manager->qLock, &mutexAttr2);  

  //pthread conditonals initalization to shared memory
  pthread_condattr_t condAttr;
  pthread_condattr_setpshared(&condAttr, PTHREAD_PROCESS_SHARED);
  pthread_cond_init(&manager->fill, &condAttr);
  pthread_condattr_t condAttr2;
  pthread_condattr_setpshared(&condAttr2, PTHREAD_PROCESS_SHARED);
  pthread_cond_init(&manager->empty, &condAttr2);

  //initalization for managers member output_f to accept the target file
  manager->output_f = fopen(argv[argc - 1], "w");
  if (!manager->output_f) {
    perror("Error Opening Output File");
    return EXIT_FAILURE;
  }
  //set for flag used later that is switched to 1 once all of the files
  //have been dealt with.
  manager->done = 0;


  /*************** end of main checks *********/

  //here I fork 2 times creating extra processes to resolve code 
  for (i = 0; i < 2; i++) {
      if(!fork()){
          resolver(manager);
          exit(0);
        }
  }

  //for loop that iterates over domain names sending them to requester 
  for (i = 1; i <= domain_names; i++) {
    requester(argv[i],manager);
  }
  //flag is set indicating that all domain names have been sent to requester
  manager->done = 1;
  //I'm done requesting!! signal fill for resolver
  pthread_cond_broadcast(&manager->fill);
  

  for (i = 0; i < 2; i++) {
    //wait for each child to finish
    wait(NULL);
  }

  //clean up
  fclose(manager->output_f);
  
  return 0;
}

/******************END OF MAIN ***************************/




void resolver(struct Manager* m) {

  char ip[MIN_IP_LENGTH];
  char url[MAX_NAME_LENGTH];

  while (1) {
      //first we lock so that other process/threads do not interfere. if the stack 
      //is empty we wait for the condition fill to be met, unless the flag done has been set
      //in that case we exit with 0
      pthread_mutex_lock(&m->qLock); // c1
      while (stack_empty(m)){ // c2
        // segFinder("2");
        if(m->done) {
          // printf("EVERYTHING RESOLVED exit 0\n");
          pthread_mutex_unlock(&m->qLock);
          exit(0);  //if all resolvers are done, by looking at flag
        }
        pthread_cond_wait(&m->fill, &m->qLock); // c3
      }
      //pop a url off the stack, then signal and unlock then later below the url is added
      //to the output file in the stack pointer.
      pop(m, url);
      // printf(" url removed = %s\n", url);
      pthread_cond_signal(&m->empty);
      pthread_mutex_unlock(&m->qLock);
      
      if (dnslookup(url, ip, sizeof(ip)) == UTIL_FAILURE) {
        fprintf(stderr, "dnslookup error: %s\n", url);
        pthread_mutex_lock(&m->fLock);
        fprintf(m->output_f, "%s,%s\n", url, "");
        pthread_mutex_unlock(&m->fLock);
      }
      else {
        pthread_mutex_lock(&m->fLock);
        fprintf(m->output_f, "%s,%s\n", url, ip);
        pthread_mutex_unlock(&m->fLock);
      } 
      
  }
  pthread_cond_broadcast(&m->empty);
}
  
  


void requester(char* input, struct Manager* m){
  //using the string passed we open the file name with said string, check it to make 
  //sure its valid.  If valid we then lock the function and add it to the stack
  //after it is added we signal fill to other processes.
  FILE* input_file = fopen(input, "r");
  if (!input_file) {
    fprintf(stderr, "Error Opening An Input File.\n");
  } else {
    
    char domain_name[MAX_NAME_LENGTH];
    
    while (fscanf(input_file, INPUTFS, domain_name) > 0) {
      pthread_mutex_lock(&m->qLock);
      //wait condition if the stack is full only is used if more than 10 domain names
      //are given to the program.
      while (stack_full(m)) {
        pthread_cond_wait(&m->empty, &m->qLock);
      } 
      // printf("added a domain\n");
      push(m, domain_name);

      pthread_cond_signal(&m->fill);
      pthread_mutex_unlock(&m->qLock);
    
    }
    
  }
  fclose(input_file);
  //return NULL;
  
}


//below is just helper functions 
void segFinder(char * x) {
  printf("seg is happening at %s\n",x);
}

/*
void printer(struct Manager *m) {
  for (int i = 0; i < 10; i++) {
    if (m->boundedBuf[i])
    printf("%s\n", m->boundedBuf[i]);
  }
}*/
