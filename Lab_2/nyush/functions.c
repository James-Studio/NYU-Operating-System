#include "functions.h"


void signal_ignore(int sig) {
    // ignore SIGINT, SIGTSTP, SIGQUIT
    if (sig == -1) {
        printf("Errors occur in the program!\n");
    }
}

void signal_child_stop(int sig) {
    pid_t p = getpid();
    if (sig == -1) {
        printf("Errors occur in the program!\n");
    }
    kill(p, SIGSTOP);
}

char* extract_basename(char* newPath) {
    char *pathToken,  *ptrSave, *returnToken;
    char* deli = "/";

    
    returnToken = strtok_r(newPath, deli, &ptrSave);

    // extract relative path consecutively 
    // happen in cd .. -> enter "/" (root dir)
    if (returnToken == NULL) {
        return "/";
    }

    while ((pathToken = strtok_r(NULL, deli, &ptrSave))) {
        returnToken = pathToken;
    }

    return returnToken;
}

char* add_user_bin(char* cmdPath) {
    char *pathToken,  *ptrSave, *returnToken;
    char* deli = "/";
    

    returnToken = strtok_r(cmdPath, deli, &ptrSave);

    // extract relative path consecutively
    if (strcmp(returnToken, "\n") == 0) {
        return "";
    }

    while ((pathToken = strtok_r(NULL, deli, &ptrSave))) {
        returnToken = pathToken;
    }


    char cmdAbs[30] = "/usr/bin/";
    strcat(cmdAbs, returnToken);

    int pl = (int) strlen(cmdAbs);

    char* retPath = (char*) malloc (pl* sizeof(char));

    strcpy(retPath, cmdAbs);

    return retPath;
}


void find_substring (char* s, char* subs,int start, int end) {
    int subi = 0;
    for (int i = start; i < end; i++) {
        subs[subi] = s[i];
        subi++;
    }
    return;
}


char* locate_relative_program (char* pth) {
    // relative path
    // int pathLen = (int) strlen(pth);
    char pathBuff[500];
    pathBuff[0] = '\0'; // init char[]

    char* absPth = (char*) malloc (500 * sizeof(char));


    // turn all the paths into absolute paths
    // parse according to /
    char** storePaths = (char**) malloc (20* sizeof(char*));
    for (int i = 0; i < 20; i++) {
        storePaths[i] = (char*) malloc(100*sizeof(char));
    }
    

    char *getPath, *stat;
    char* slashDeli = "/";
    
    getPath = strtok_r(pth, slashDeli, &stat);
    storePaths[0] = getPath;
    //printf("here: %s\n", storePaths[0]);

    int count = 1;
    while ((getPath = strtok_r(NULL, slashDeli, &stat))) {
        storePaths[count] = getPath;
        //printf("here: %s\n", storePaths[count]);
        count++;
    }
    
    for (int i = 0; i < count - 1; i++) {
        chdir(storePaths[i]);
    }

    strcpy(absPth, getcwd(pathBuff, 500));
    strcat(absPth, "/");
    strcat(absPth, storePaths[count-1]);


    return absPth;
}



// clean operations
char* clean_last_spaces (char* cmdLine) {
    int slen = ((int) strlen(cmdLine)) - 2;
    
    // find an exception
    // the last char in cmdLine is '\n'
    if (cmdLine[slen+1] == '\n') {
        cmdLine[slen+1] = '\0';
    }


    // remove redundant spaces
    while (cmdLine[slen] == ' ') {
        cmdLine[slen] = '\0';
        slen--;
    }
 
   
    return cmdLine;
}

void clean_memory (char** argList, int limit) {
    for (int n = 0; n < limit; n++) {
        argList[n] = NULL;
    }
    return;
}

void clean_jobs_mem (recordJB* jb, int limit) {
    for (int i = 0; i < limit; i++) {
        jb[i].job_mid = -1;
        jb[i].job_pid = -1;
        jb[i].line[0] = '\0'; // init with '/0' or NULL
    }
}

char* create_jobs_command(char* cmdLine, int cpID) {
    // copy the command line in a string
    int lenInt = 0;
    int n = cpID;
    while (n / 10 != 0) {
        n /= 10;
        lenInt += 1;
    }

    
    char strInt[lenInt+1];
    sprintf(strInt, "%d", cpID);

    // build the string that meets the HW requirement
    int cl = strlen(cmdLine);
    int jl = lenInt + cl + 3;

    // build customized string
    char jobsLine[jl];
    jobsLine[0] = '\0'; // remember to declare the string is empty!
    strcat(jobsLine, "[");
    strcat(jobsLine, strInt);
    strcat(jobsLine, "] ");
    strcat(jobsLine, cmdLine);


    char* returnCmd = (char*) malloc(jl * sizeof(char));

    strcpy(returnCmd, jobsLine);

    return returnCmd;
}