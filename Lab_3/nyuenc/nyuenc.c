#include "functions.h"

/* Locks */
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t job_not_empty = PTHREAD_COND_INITIALIZER;
pthread_cond_t res_not_empty = PTHREAD_COND_INITIALIZER;


// Encoding Algorithm for the "Threads" (Consumer)
void *encode_string(void *targ) {
  // infinite loop (Because thread pool)
  for (;;) {
    threadArg *newTarg = (threadArg*) targ; // change the type back
    
    pthread_mutex_lock(&mutex);
    
    // if the count of the chunk == 0, the program should wait
    while (((newTarg->thrdJL)->startLoc >= (newTarg->thrdJL)->endLoc) && (newTarg->thrdJL->finishProd == false)) {
      pthread_cond_wait(&job_not_empty, &mutex);
    }

    //
    while (((newTarg->thrdJL)->startLoc >= (newTarg->thrdJL)->endLoc) && (newTarg->thrdJL->finishProd == true)) {
      pthread_mutex_unlock(&mutex);
      pthread_exit(NULL);
    }
    
    // get chunk ID & file ID
    int getStart = (newTarg->thrdJL)->startLoc;
    // get chunk ID & file ID
    int getFileID = (newTarg->thrdJQ)[getStart].fid;
    
    // add one to the start chunk (affect other threads)
    (newTarg->thrdJL)->startLoc++;

    int countCons = 0, countEncode = 0;
    char currentChar;
    unsigned char *encodeStr = (unsigned char*) malloc (10000 * sizeof(unsigned char));

    // use chunk size to get the binary data
    for (int strCount = (newTarg->thrdJQ)[getStart].cloc * 4096; strCount < ((newTarg->thrdJQ)[getStart].cloc * 4096 + (newTarg->thrdJQ)[getStart].csize); strCount++) {
      // extract the data
      // Case 1: Change to the new character && No count
      if (countCons == 0) {
        currentChar = newTarg->dataList[getFileID][strCount];
        countCons++;
      }
      // Case 2: Change to the new character && Have count
      // Add characters in the encoded string
      else if (currentChar != (newTarg->dataList[getFileID][strCount])) {
        // Assign the char and number in the string
        encodeStr[countEncode] = currentChar;
        //printf("encodeStr[countEncode]: %c\n", encodeStr[countEncode]);
        encodeStr[countEncode+1] = countCons;
        //printf("encodeStr[countEncode+1]: %d\n", (int) encodeStr[countEncode+1]);
        countEncode += 2;

        // Change to the new character
        currentChar = newTarg->dataList[getFileID][strCount];
        countCons = 1;
      }
      // Case Test
      else if (countCons >= 255) {
        encodeStr[countEncode] = currentChar;
        encodeStr[countEncode+1] = countCons;
        countEncode += 2;
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
    //printf("encodeStr[countEncode]: %c\n", encodeStr[countEncode]);
    //printf("encodeStr[countEncode+1]: %d\n", countCons);
    //printf("encodeStr[countEncode+1]: %d\n", encodeStr[countEncode+1]);
    //encodeStr[countEncode+2] = '\0';

    // Still update the recording value
    countCons = 0;
    countEncode += 2;
    //countEncode += 3;

    //
    int resLoc = getStart;

    (newTarg->thrdRQ)[resLoc].chunkid = (newTarg->thrdJQ)[resLoc].chunkid;
    (newTarg->thrdRQ)[resLoc].result = encodeStr;
    (newTarg->thrdRQ)[resLoc].resLen = countEncode;

    newTarg->resCount++;

    // Send the signal back
    pthread_cond_signal(&res_not_empty); 
    
    // Unlock the mutex
    pthread_mutex_unlock(&mutex);
  }
  pthread_exit(NULL);
}

/* -------------------------------------------------------------------------------------------------------------- */

int main(int argc, char **argv) {
  /*  
    For this command: 
    ./nyuenc file.txt file.txt > file2.enc (argc == 3) -> So the number of files equal to (argc - 1)
    ./nyuenc -j 3 file.txt file.txt > file2.enc (argc == 5) -> need to use argcStart to find the files location in argv
    
    ** Remember: No copying in the data (Slow the program down) **
  */ 

  /*
    References: 
    Multithreading: https://www.geeksforgeeks.org/multithreading-in-c/
    getopt(): https://www.gnu.org/software/libc/manual/html_node/Example-of-Getopt.html
    OS Lecture 2-proc.pdf page 262: The producer-consumer problem with pthread
  */

  // The main thread should divide the input data into fixed-size chunks
  int fd;
  int jobQueueLen = 0;

  struct stat *sb = (struct stat*) malloc((argc-1)* sizeof(struct stat));
  char **addrList = (char**) malloc((argc-1) * sizeof(char*));
  rckChunks *locList = (rckChunks*) malloc((argc-1) * sizeof(rckChunks));
  long long int getFileAlloc = 0;

  /* Find Thread Numbers */
  // If the argument contain -j, remember to change the starting position of the argument to meet (Use argcStart)
  int threadNums = -1, argcStart = 1;
  int opt;
  while ((opt = getopt(argc, argv, "j:")) != -1) {
      switch(opt) {
        case 'j':
          threadNums = atoi(optarg);
          argcStart = 3;
          break; 
        default: 
          threadNums = 1;
          break;
      }
  }
  
  if (threadNums < 0) {
    threadNums = 1;
  }
  /* ------------------------------------------------------------------------------------------------------------------------------------------------ */

  // allocate memory for the job queue 
  pthread_t tid[threadNums];

  jbElements* jobQueue = (jbElements*) malloc(200000000 * sizeof(jbElements)); // record the position of the chunk (So no need to copy the data)
  jbQueueLoc* jobQueueLoc = (jbQueueLoc*) malloc(sizeof(jbQueueLoc));

  // store the results in the resElements (length equals to jobQueueLen)
  resElements *resQueue = (resElements*) malloc(200000000 * sizeof(resElements));

  // init jobQueue
  jobQueueLoc->startLoc = 0;
  jobQueueLoc->endLoc = 0;
  jobQueueLoc->finishProd = false;

  // init arguments for the thread function
  threadArg *targ = (threadArg*) malloc (sizeof(threadArg));
  targ->fileNum = argc-1;
  targ->dataList = addrList;
  targ->thrdJQ = jobQueue;
  targ->thrdJL = jobQueueLoc;
  targ->thrdRQ = resQueue;
  targ->resCount = 0;
  targ->jobCount = 0;

  // call the thread pools
  for (int t = 0; t < threadNums; t++) {
    pthread_create(&tid[t], NULL, encode_string, targ);
  }

  // -------------------------------------------------------------------------------------------------------------------------------------------------
  
  int countj = 0;
  // Map multiple files into the memory
  for (int f = 0, cpStart = argcStart; f < (argc-1) && cpStart < argc; f++, cpStart++) {
    fd = open(argv[cpStart], O_RDONLY);
    
    // check fd
    if (fd == -1) {
      /*fwrite("NO", sizeof(char), 0, stdout);
      fflush(stdout);*/
      //fprintf(stderr, "Errors in fd\n");
      return 1;
    }

    // get file size
    if (fstat(fd, &sb[f]) == -1) {
      /*fwrite("NO", sizeof(char), 0, stdout);
      fflush(stdout);*/
      //fprintf(stderr, "Errors in fstat\n");
      return 1;
    }

    // map files into memory
    // Use sb[f].st_size to get full size and use 4096 to divide the chunks further
    addrList[f] = mmap(NULL, sb[f].st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    getFileAlloc += sb[f].st_size;
    
    int remainChunk = ((int) sb[f].st_size) % 4096;
    
    // Be aware of nChunks (only add one to the "numChunk" when remainChunk is larger than 1)
    int nChunks;
    if (remainChunk > 0) {
      nChunks = ((int) sb[f].st_size) / 4096 + 1;
    }
    else {
      nChunks = ((int) sb[f].st_size) / 4096;
    }
    
    jobQueueLen += nChunks;
    //printf("nChunks: %d\n", nChunks);

    locList[f].numChunk = nChunks;
    locList[f].lastChunk = remainChunk;
    
    // insert processed chunks 
    for (int nc = 0; nc < locList[f].numChunk; nc++) {
      jobQueue[countj].chunkid = countj + 1; // the id of chunk in all files (start from 1)
      jobQueue[countj].fid = f;
      jobQueue[countj].cloc = nc; // the id of chunk in one file
      //
      if (locList[f].lastChunk > 0 && nc == locList[f].numChunk - 1) {
        jobQueue[countj].csize = locList[f].lastChunk;
      }
      else {
        jobQueue[countj].csize = 4096;
      }
      countj++;
      jobQueueLoc->endLoc++;
      targ->jobCount++;
      pthread_cond_signal(&job_not_empty);
    }

    if (addrList[f] == MAP_FAILED) {
      /*fwrite("NO", sizeof(char), 0, stdout);
      fflush(stdout);*/
      //fprintf(stderr, "Errors in mmap\n");
      return 1;
    }

    close(fd);
  }
  jobQueueLoc->finishProd = true;
  pthread_cond_signal(&job_not_empty);


  // Result Collection Phase
  
  // arrange the result queue here (resQueueLen == jobQueueLen)
  // assume the resQueue is already stored in the order
  int countResID = 0;

  // unsigned char *ansString = (unsigned char*) malloc((getFileAlloc * argc * 2) * sizeof(unsigned char));
  unsigned char *ansString = (unsigned char*) malloc((2000000000) * sizeof(unsigned char));

  unsigned char lastChar;
  int lastNum = 0;

  /*for (int t = 0; t < threadNums; t++) {
    pthread_join(tid[t], NULL);
  }*/

  int r = 0; // current r (record)
  for (; r < jobQueueLen; r++) {
    // if res is larger than the jobQueueLen
    pthread_mutex_lock(&mutex);
    while (targ->resCount <= r) {
      pthread_cond_wait(&res_not_empty, &mutex);
    }
    pthread_mutex_unlock(&mutex);
    
    
    // get res number
    // printf("targ->resCount: %d\n", targ->resCount);

    for (int countChar = 0; countChar < 10000; countChar++) {
      // testing code
      /*if ((resQueue[r].result)[countChar] == '\0') {
          printf("bomb\n");
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
      // formal code (arrange all the strings in the resQueue into one complete string)
      int getResLen = resQueue[r].resLen;
      //if (lastChar != '\0' && lastNum != 0 && countChar == 0) {
      if (lastNum != 0 && countChar == 0) {
        if (lastChar == (resQueue[r].result)[countChar] && (lastNum + ((int) (resQueue[r].result)[countChar + 1])) < 256) {
          //ansString[countResID] = lastChar;
          //ansString[countResID + 1] = lastNum + (int) (resQueue[r].result)[countChar + 1];
          lastNum = lastNum + ((int) (resQueue[r].result)[countChar + 1]);
          //countResID += 2;
          //
          if (getResLen <= 2) {
            break;
          }
          else {
            ansString[countResID] = lastChar;
            ansString[countResID + 1] = lastNum;
            //ansString[countResID + 2] = (resQueue[r].result)[countChar];
            //ansString[countResID + 3] = (resQueue[r].result)[countChar + 1];
            countResID += 2;
            countChar++; // not "countChar += 2"
          }
        }
        else if (lastChar == (resQueue[r].result)[countChar] && (lastNum + ((int) (resQueue[r].result)[countChar + 1])) > 255) {
          ansString[countResID] = lastChar;
          ansString[countResID + 1] = 255;
          lastNum = lastNum + ((int) (resQueue[r].result)[countChar + 1]) - 255;
          countResID += 2;
          //
          if (getResLen <= 2) {
            break;
          }
          else {
            ansString[countResID] = lastChar;
            ansString[countResID + 1] = lastNum;
            //ansString[countResID + 2] = (resQueue[r].result)[countChar];
            //ansString[countResID + 3] = (resQueue[r].result)[countChar + 1];
            countResID += 2;
            countChar++;
          }
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
      
      //else if ((resQueue[r].result)[countChar + 2] == '\0') {
      else if (countChar >= (getResLen - 2)) {
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

  //if (lastChar != '\0') {
  if (lastNum != 0) {
    ansString[countResID] = lastChar;
    ansString[countResID + 1] = lastNum;
    countResID += 2;
  }

  // Wait for all the threads
  /*for (int t = 0; t < threadNums; t++) {
    pthread_join(tid[t], NULL);
  }*/
  
  // Write data to the STDOUT
  // Use countEncode as the size of the string written into the encoded file
  fwrite(ansString, sizeof(char), countResID, stdout);
  fflush(stdout);
  free(ansString);
}
