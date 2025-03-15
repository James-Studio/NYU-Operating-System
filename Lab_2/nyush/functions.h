#ifndef _FUNCTIONS_H_
#define _FUNCTIONS_H_

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>

typedef struct rcdIO{
    int loc_i;
    int loc_o;
    int loc_ii;
    int loc_oo;
} recordIO;

typedef struct rcdFD {
    int fdr;
    int fdw;
    int fdww;
} recordFD;

typedef struct rcdJB {
    int job_mid;
    pid_t job_pid;
    char line[1000];
} recordJB;

/* Functions */
void signal_ignore(int sig);
void signal_child_stop(int sig);

void clean_memory (char** argList, int limit);
void clean_jobs_mem (recordJB* jb, int limit);
char* clean_last_spaces (char* cmdLine);
char* extract_basename(char* newPath);
char* add_user_bin(char* cmdPath);
char* create_jobs_command(char* cmdLine, int cpID);
char* locate_relative_program (char* pth);
void find_substring (char* s, char* subs,int start, int end);

#endif