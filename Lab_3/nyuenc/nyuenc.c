#include "functions.h"

/* Locks */
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
// pthread_cond_t condVar = PTHREAD_COND_INITIALIZER;

// Encoding Algorithm for the "Threads"
void *encode_string(void *targ) {
  // infinite loop
  for (;;) {
    pthread_mutex_lock(&mutex);
    //pthread_t tidThis = pthread_self();
    //printf("This thread: %ld\n", tidThis);
    int countCons = 0, countEncode = 0;
    char currentChar;
    unsigned char *encodeStr = (unsigned char*) malloc (10000 * sizeof(unsigned char));
    
    threadArg *newTarg = (threadArg*) targ; // change the type back

    // if the queue is empty now, exit the thread (because the queue will not have new chunks)
    while ((newTarg->thrdJL)->startLoc > (newTarg->thrdJL)->endLoc) {
      pthread_mutex_unlock(&mutex);
      pthread_exit(NULL);
    }
    // get chunk ID & file ID
    int getChunkID = (newTarg->thrdJL)->startLoc;
    int getFileID = (newTarg->thrdJQ)[getChunkID].fid;

    // use chunk size to get the binary data
    for (int strCount = (newTarg->thrdJQ)[getChunkID].cloc * 4096; strCount < ((newTarg->thrdJQ)[getChunkID].cloc * 4096 + (newTarg->thrdJQ)[getChunkID].csize); strCount++) {
      // extract the data
      // Case 1: Change to the new character && No count
      if (countCons == 0) {
        currentChar = newTarg->dataList[getFileID][strCount];
        countCons++;
      }
      // Case 2: Change to the new character && Have count
      // Add characters in the encoded string
      else if (currentChar != (newTarg->dataList[getFileID][strCount])) {
        //printf("currentChar: %c\n", currentChar);
        //printf("countCons: %d\n", countCons);

        // Assign the char and number in the string
        encodeStr[countEncode] = currentChar;
        //printf("encd: %c\n", encodeStr[countEncode]);

        encodeStr[countEncode+1] = countCons;
        //printf("countcons: %d\n", countCons);
        //printf("encd: %d\n", (int) encodeStr[countEncode+1]);

        countEncode += 2;

        // Change to the new character
        currentChar = newTarg->dataList[getFileID][strCount];
        countCons = 1;
      }
      // Case 3: Remain the previous character
      else {
        countCons++;
      }
    }
    
    // Assign the last character and number to the encoded string
    encodeStr[countEncode] = currentChar;
    encodeStr[countEncode+1] = countCons;
    encodeStr[countEncode+2] = '\0';
    // Still update the recording value
    countCons = 0;
    countEncode += 3;
   
    int resLoc = (newTarg->thrdJL)->startLoc;
    (newTarg->thrdRQ)[resLoc].chunkid = (newTarg->thrdJQ)[resLoc].chunkid;
    (newTarg->thrdRQ)[resLoc].result = encodeStr;
    //fwrite(encodeStr, sizeof(char), countEncode, stdout);
    fflush(stdout);

    /* ---------------------------------------------------- */

    // add one to the start chunk (affect other threads)
    (newTarg->thrdJL)->startLoc++;
    // printf("startLoc: %d\n", (newTarg->thrdJL)->startLoc);
    // printf("endLoc:: %d\n", (newTarg->thrdJL)->endLoc);
    
    pthread_mutex_unlock(&mutex);
  }
  pthread_exit(NULL);
}


/* -------------------------------------------------------------------------------------------------------------- */

int main(int argc, char **argv) {
  /*  Milestone 1: read files from one or more

    For this command: ./nyuenc file.txt file.txt > file2.enc (argc == 3)
    So the number of files equal to (argc - 1)

  * Remember: No copying in the data (Slow the program down)
  * 
  * 
  */ 

  // Sample Command in Milestone 1: ./nyuenc file.txt file.txt > file2.enc
  // Open file
  // ** Remember to free allocated memories!

  /*
    References: 
    Multithreading: https://www.geeksforgeeks.org/multithreading-in-c/
    2-proc.pdf page 262: The producer-consumer problem with pthread
  */

  // The main thread should divide the input data into fixed-size chunks
  int fd;
  int jobQueueLen = 0;

  struct stat *sb = (struct stat*) malloc((argc-1)* sizeof(struct stat));
  char **addrList = (char**) malloc((argc-1) * sizeof(char*));
  rckChunks *locList = (rckChunks*) malloc((argc-1) * sizeof(rckChunks));
  size_t getFileAlloc;

  for (int f = 0; f < (argc-1); f++) {
    fd = open(argv[f+1], O_RDONLY);
    
    if (fd == -1) {
      fprintf(stderr, "Errors in fd\n");
      return 1;
    }

    // Get file size
    if (fstat(fd, &sb[f]) == -1) {
      fprintf(stderr, "Errors in fstat\n");
      return 1;
    }

    // Map file into memory
    // sb[f].st_size or 4096 (have to make sure)
    addrList[f] = mmap(NULL, sb[f].st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    getFileAlloc = sb[f].st_size;
    
    int remainChunk = (int) (sb[f].st_size % 4096);
    int nChunks = (int) (sb[f].st_size / 4096) + 1;
    
    jobQueueLen += nChunks;
    
    locList[f].numChunk = nChunks;
    locList[f].lastChunk = remainChunk;
    
    if (addrList[f] == MAP_FAILED) {
      fprintf(stderr, "Errors in mmap\n");
      return 1;
    }

    close(fd);
  }
  
  
  // allocate memory for the job queue
  jbElements* jobQueue = (jbElements*) malloc(jobQueueLen * sizeof(jbElements));
  jbQueueLoc* jobQueueLoc = (jbQueueLoc*) malloc(sizeof(jbQueueLoc));

  // store the results in the resElements (length equals to jobQueueLen)
  resElements *resQueue = (resElements*) malloc(jobQueueLen * sizeof(resElements));
  /*for (int r = 0; r < jobQueueLen; r++) {
    resQueue[r].result = (unsigned char*) malloc (10000 * sizeof(unsigned char));
  }*/
  

  // init jobQueueLoc and jobQueue
  // build job queue (insert type jbElements)
  int countj = 0;
  for (int f = 0; f < (argc-1); f++) {
    for (int nc = 0; nc < locList[f].numChunk; nc++) {
      jobQueue[countj].chunkid = countj + 1; // the id of chunk in all files (start from 1)
      jobQueue[countj].fid = f;
      jobQueue[countj].cloc = nc; // the id of chunk in one file
      //
      if (nc == locList[f].numChunk - 1) {
        jobQueue[countj].csize = locList[f].lastChunk;
      }
      else {
        jobQueue[countj].csize = 4096;
      }
      countj++;
    }
  }

  jobQueueLoc->startLoc = 0; // exact location in the array (pop from here)
  jobQueueLoc->endLoc = jobQueueLen - 1;
  // printf("endloc: %d %d\n", jobQueueLen, jobQueueLoc->endLoc);
  // exit(0);
  // validate
  // validateJobQueue(jobQueueLen, jobQueue, addrList);

  /*
     Your code must be free of race conditions. You must not perform busy waiting
     , and you must not use sleep(), usleep(), or nanosleep().
  */

  /* Create Thread Pools */
  int threadNums = -1;
  int opt;
  while ((opt = getopt(argc, argv, "j:")) != -1) {
      switch(opt) {
        case 'j':
          threadNums = atoi(optarg);
          break; 
        default: 
          threadNums = 1;
          break;
      }
  }
  
  if (threadNums < 0) {
    threadNums = 1;
  }

  pthread_t tid[threadNums];
  threadArg *targ = (threadArg*) malloc (sizeof(threadArg));
  
  // init arguments for the thread function
  targ->fileNum = argc-1;
  targ->dataList = addrList;
  targ->thrdJQ = jobQueue;
  targ->thrdJL = jobQueueLoc;
  targ->thrdRQ = resQueue;

  // Asssume the number of the thread first
  for (int t = 0; t < threadNums; t++) {
    pthread_create(&tid[t], NULL, encode_string, targ);
  }

  // wait for all the threads
  void *ret_val;
  for (int t = 0; t < threadNums; t++) {
    pthread_join(tid[t], &ret_val);
  }
  
  // Arrange the result queue here (resQueueLen == jobQueueLen)
  // Assume the resQueue is already stored in the order
  int countResID = 0;
  unsigned char *ansString = (unsigned char*) malloc((getFileAlloc * 2) * sizeof(unsigned char));
  
  char lastChar = '\0';
  int lastNum = 0;
  
  for (int r = 0; r < jobQueueLen; r++) {
    for (int countChar = 0; countChar < 10000; countChar++) {
      // testing code
      /*if ((resQueue[r].result)[countChar] == '\0') {
          break;
      }
      else if (countChar % 2 == 0) {
          printf("%c\n", (resQueue[r].result)[countChar]);
          fflush(stdout);
      }
      else {
          printf("%d\n", (int)(resQueue[r].result)[countChar]);
          fflush(stdout);
      }*/

      /* ---------------------------------------------------------- */
      // formal code
      if (lastChar != '\0' && lastNum != 0 && countChar == 0) {
        if (lastChar == (resQueue[r].result)[countChar]) {
          ansString[countResID] = lastChar;
          ansString[countResID + 1] = lastNum + (int) (resQueue[r].result)[countChar + 1];
          countResID += 2;
          countChar++; // not "countChar += 2"
        }
        else {
          ansString[countResID] = lastChar;
          ansString[countResID + 1] = lastNum;
          ansString[countResID + 2] = (resQueue[r].result)[countChar];
          ansString[countResID + 3] = (resQueue[r].result)[countChar + 1];
          countResID += 4;
          countChar++;
        }
      }
      else if ((resQueue[r].result)[countChar + 2] == '\0') {
        lastChar = (resQueue[r].result)[countChar];
        lastNum = (int) (resQueue[r].result)[countChar + 1];
        break;
      }
      else if (countChar % 2 == 0) {
        ansString[countResID] = (resQueue[r].result)[countChar];
        countResID++;
      }
      else {
          // countChar % 2 != 0
          ansString[countResID] = (int) (resQueue[r].result)[countChar];
          countResID++;
      }
    }
  }

  if (lastChar != '\0') {
    ansString[countResID] = lastChar;
    ansString[countResID + 1] = lastNum;
    countResID += 2;
  }
  

  // Write data to the STDOUT
  // Use countEncode as the size of the string written into the encoded file
  
  fwrite(ansString, sizeof(char), countResID, stdout);
  fflush(stdout);

  /* -------------------------------------------------------------------------------------------------------------- */

}
