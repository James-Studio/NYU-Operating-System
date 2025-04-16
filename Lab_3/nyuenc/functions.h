#ifndef _FUNCTIONS_H_
#define _FUNCTIONS_H_

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>


/* Structures */
typedef struct rcdChk {
    int numChunk;
    int lastChunk;
} rckChunks;

typedef struct jbEle {
    int chunkid; // for arranging the sequence of the data
    int fid; // id of files
    int cloc; // location in the chunk (use fid to find)
    int csize; // size of the chunk
} jbElements;

typedef struct resEle {
    int chunkid;
    int resLen;
    unsigned char *result;
} resElements;

typedef struct jbQue {
    int startLoc; 
    int endLoc;
    bool finishProd;
} jbQueueLoc;

typedef struct thrdArg {
    int fileNum;
    char **dataList;
    int resCount;
    int jobCount;
    jbElements *thrdJQ; // jobQueue
    jbQueueLoc *thrdJL; // jobLocs
    resElements *thrdRQ; // resQueue
} threadArg;

/* Functions */
// jobQueue is unbounded (No need to check full condition)
int empty_job_queue(jbQueueLoc* jobQ);


// validation tool (Not necessary for the main code)
// validate whether the data is stored in the desired location
void validate_job_queue(int jobQueueLen, jbElements* jobQueue, char **addrList);


#endif
