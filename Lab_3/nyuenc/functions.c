#include "functions.h"

// jobQueue is unbounded (No need to check full condition)
int empty_job_queue(jbQueueLoc* jobQ) {
    if (jobQ->startLoc > jobQ->endLoc) {
        return 1; // true if empty
    }
    else {
        return 0;
    }
}


// validation tool (Not necessary for the main code)

// validate whether the data is stored in the desired location
void validate_job_queue(int jobQueueLen, jbElements* jobQueue, char **addrList) {
    for (int i = 0; i < jobQueueLen; i++) {
        for (int j = jobQueue[i].cloc * 4096; j < (jobQueue[i].cloc * 4096 + jobQueue[i].csize); j++) {
          printf("j: %d\n", j);
          printf("jobQueue[%d]: %c\n", i, addrList[jobQueue[i].fid][j]);
        }
    }
}

