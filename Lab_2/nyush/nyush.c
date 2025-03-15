#include "functions.h"

/*
Commands:
1. cd <dir>
2. jobs
3. fg <index>
4. exit
*/


/*
# Reference Websites
StackOverFlow:
https://stackoverflow.com/questions/15961253/c-correct-usage-of-strtok-r

GeeksForGeeks:
https://www.geeksforgeeks.org/input-output-system-calls-c-create-open-close-read-write/
https://www.geeksforgeeks.org/dup-dup2-linux-system-call/
*/


int main() {
    int MAX_ARGS = 50;
    int MAX_SUSJOBS = 200;

    char *cmdLine, *baseName, *currPath;

    // for jobs command
    int trackSuspendIDs = 1;
    recordJB* trackSuspendJobs;


    // directory related variables (for getcwd())
    char pathBuff[500];

    // Allocate memories
    cmdLine = (char*) malloc(200 * (sizeof(char)));
    baseName = (char*) malloc(200 * (sizeof(char)));
    currPath = (char*) malloc(200 * (sizeof(char)));
    
    
    trackSuspendJobs = (recordJB*) malloc(MAX_SUSJOBS * (sizeof(recordJB)));
    clean_jobs_mem(trackSuspendJobs, MAX_SUSJOBS);

    // init the baseName
    strcpy(currPath, getcwd(pathBuff, 500));
    char* catchBaseName = extract_basename(currPath); // basenaeme: terminal location
    strcpy(baseName, catchBaseName);

    signal(SIGTSTP, signal_ignore);
    signal(SIGINT, signal_ignore);
    signal(SIGQUIT, signal_ignore);
   

    // baseName = "/";
    while (1) {

        /* -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- */
        // print out and get the prompts
        printf("[nyush %s]$ ",baseName);
        fflush(stdout);

        size_t cmdSize = 100;
        if (getline(&cmdLine, &cmdSize, stdin) == -1) {
            free(cmdLine);
            free(baseName);
            free(currPath);
            free(trackSuspendJobs);
            break;
        }
        

        // if the command line contains nothing
        if (strcmp(cmdLine, "\n") == 0) {
            continue;
        }
        if (cmdLine[0] == '|') {
            fprintf(stderr, "Error: invalid command\n");
            continue;
        }
        
        // clean spaces at the end
        // replace redundant spaces with \0
        cmdLine = clean_last_spaces(cmdLine); 
        // record the command line for jobs
        char* trackSuspendLine = create_jobs_command(cmdLine, trackSuspendIDs);

        // parse the string
        char *cmdTokens, *ptrSave, *ptrSave2;
        char *dSpace = " ", *dPipe = "|";
        
        char* allTokens = (char*) malloc (((int) strlen(cmdLine) + 1) * sizeof(char)); // !!!problem

        //char* allTokens = cmdLine;

        strcpy(allTokens, cmdLine);
        
        char*** argList = (char***) malloc (10 * sizeof(char**));
    

        
        /* 
            Follow this structure: (Deconstruct char***)
            char*** allCmds = (char***) malloc (10 * sizeof(char**));
            char** sigCmd = (char**) malloc (50 * sizeof(char**));
            char* allwrds = (char*) malloc (50 * sizeof(char));
        */
        // allocate memories for char*** 
        for (int i = 0; i < 10; i++) {
            argList[i] = (char**) malloc (50 * sizeof(char*));
            for (int j = 0; j < 50; j++) {
                argList[i][j] = (char*) malloc (50 * sizeof(char));
            }
        }
        // clean memories in storing each cmd
        for (int i = 0; i < 10; i++) {
            clean_memory(argList[i], MAX_ARGS); 
        }

        // Use strtok_r() to parse the string 
        int countStr = 1;
        int countSing = 1; 
        char* charTokens;
        cmdTokens = strtok_r(allTokens, dPipe, &ptrSave);

        argList[0][0] = strtok_r(cmdTokens, dSpace, &ptrSave2);

        while ((charTokens = strtok_r(NULL, dSpace, &ptrSave2))) {
            argList[0][countSing] = charTokens;
            countSing++;
        }

        countSing = 1;
        while ((cmdTokens = strtok_r(NULL, dPipe, &ptrSave))) {
            charTokens = strtok_r(cmdTokens, dSpace, &ptrSave2);
            argList[countStr][0] = charTokens;
    
            while ((charTokens = strtok_r(NULL, dSpace, &ptrSave2))) {
                argList[countStr][countSing] = charTokens;
                countSing++;
            }
            countStr++;
            countSing = 1;
        }
        


        // extract pipe for a single line

        int pipeNum = countStr - 1;

        // !!! CHILD PROCESSES !!!
        // create new threads and execute new programs
        // create multiple child process simultaneously

        // build pipes
        int mulPipe[pipeNum][2]; 
        
        if (pipeNum > 0) {
            for (int pi = 0; pi < pipeNum; pi++) {
                pipe(mulPipe[pi]);
            }
        }
        
        // I put the four commands specifically assigned in the Lab in the parent process !!
        if (strcmp(argList[0][0], "cd") != 0 && strcmp(argList[0][0], "exit") != 0 && strcmp(argList[0][0], "jobs") != 0 && strcmp(argList[0][0], "fg") != 0) 
        {   
            int returnStat;
            
            for (int eachCmd = 0; eachCmd <= pipeNum; eachCmd++) {
                pid_t pid = fork();
                

                if (pid < 0) {
                    // fail to create the process
                    fprintf(stderr, "Error: Fail to create new processes!\n");
                    exit(1);
                }
                else if (pid == 0) {
                    // Child process
                    signal(SIGTSTP, signal_child_stop);

                    
                    /*
                    Example for file descriptors: cmd 1 | cmd2 | cmd 3
                    // Make it easier to program in the following
                        0 (cmd 1)
                        [0][1]

                        1 (cmd 2)
                        [0][0]
                        [1][1]

                        2 (cmd 3)
                        [1][0]
                    */
                   //
                    if (pipeNum > 0) {
                        if (eachCmd > 0) {
                            dup2(mulPipe[eachCmd-1][0], 0);
                        }
                        if (eachCmd < pipeNum) {
                            dup2(mulPipe[eachCmd][1], 1);
                        }


                        // close all the pipes in each child process
                        for (int i = 0; i < pipeNum; i++) {
                            close(mulPipe[i][0]);
                            close(mulPipe[i][1]);
                        }
                    }

                    // redirect the input and output if special characters is met
                    int getSize = 0, listSize = 0;

                    // retrieve the size of argList
                    while (argList[eachCmd][getSize]) {
                        if (strcmp(argList[eachCmd][getSize], ">") == 0 || strcmp(argList[eachCmd][getSize], "<") == 0 
                        || strcmp(argList[eachCmd][getSize], ">>") == 0 || strcmp(argList[eachCmd][getSize], "<<") == 0) {
                            break;
                        }
                        getSize++;
                    } 

                    while (argList[eachCmd][listSize]) {
                        listSize++;
                    }
                    
                    
                    
                    // !!! FOR "/usr/bin" COMMANDS or "CUSTOMED FILES"!!!

                    // if the commands enter this part, follow the case rules below!
                    
                    // free cmdOriginal
                    char* cmdOriginal = (char*) malloc( ((int) strlen(argList[eachCmd][0]) + 1) * sizeof(char));
                    strcpy(cmdOriginal, argList[eachCmd][0]); // for args[0] because argList[eachCmd][0] will disappear some chars
                    
                    char cmdFullLine[50]; // for unknown cmds, divide them into the cases below3

                    // if the str does not contain '/'
                    // case 1 (need to add /usr/bin)
                    if (!strchr(argList[eachCmd][0], '/')) {
                        strcpy(cmdFullLine, add_user_bin(argList[eachCmd][0]));
                    }
                    // case 2 (absolute path)
                    else if (argList[eachCmd][0][0] == '/') {
                        strcpy(cmdFullLine, argList[eachCmd][0]);
                    }
                    // case 3 (relative path)
                    else {
                        //char copyPath[] = argList[eachCmd][0];
                        strcpy(cmdFullLine, locate_relative_program(argList[eachCmd][0]));
                    }
                    
                    
                    // read args dynamically
                    char* args[getSize+1]; // +1 : for NULL
                    args[0] = cmdOriginal;
                    
                    for (int i = 1; i < getSize; i++) {
                        args[i] = argList[eachCmd][i];
                    }
                    args[getSize] = NULL; // Remember to add NULL
                    
                    //
                    recordIO IOtable;

                    IOtable.loc_i = -1;
                    IOtable.loc_o = -1;
                    IOtable.loc_ii = -1;
                    IOtable.loc_oo = -1;
                    
                    for (int i = 0; i < listSize; i++) {
                        if (strcmp(argList[eachCmd][i], ">") == 0) {
                            IOtable.loc_i = i;
                        }
                        else if (strcmp(argList[eachCmd][i], "<") == 0) {
                            IOtable.loc_o = i;
                        }
                        else if (strcmp(argList[eachCmd][i], ">>") == 0) {
                            IOtable.loc_ii = i;
                        }
                        else if (strcmp(argList[eachCmd][i], "<<") == 0) {
                            IOtable.loc_oo = i;
                        }
                    }
                    
                    recordFD fds;

                    fds.fdr = -1;
                    fds.fdw = -1;
                    fds.fdww = -1;

                    for (int i = 0; i < listSize; i++) {
                        // >
                        if (IOtable.loc_i == i) {
                            if (listSize - 1 > IOtable.loc_i) {
                                fds.fdw = open(argList[eachCmd][IOtable.loc_i + 1], O_WRONLY | O_CREAT | O_TRUNC);
                                
                                dup2(fds.fdw, 1);

                                close(fds.fdw);
                            }
                            else {
                                fprintf(stderr, "Error: invalid command\n");
                                exit(1);
                            }
                        }
                        // <
                        else if (IOtable.loc_o == i) {
                            if (listSize - 1 > IOtable.loc_o) {
                                fds.fdr = open(argList[eachCmd][IOtable.loc_o+1], O_RDONLY);
                                // error occurs
                                if (fds.fdr == -1) {
                                    fprintf(stderr, "Error: invalid file\n");
                                    close(fds.fdr);
                                    exit(1);
                                }
                                dup2(fds.fdr, 0);

                                close(fds.fdr);
                            }
                            else {
                                fprintf(stderr, "Error: invalid command\n");
                                exit(1);
                            }
                        }
                        // >>
                        else if (IOtable.loc_ii == i) {
                            if (listSize - 1 > IOtable.loc_ii) {
                                fds.fdww = open(argList[eachCmd][IOtable.loc_ii + 1], O_WRONLY | O_APPEND | O_CREAT);
                                dup2(fds.fdww, 1);

                                close(fds.fdww);
                            }
                            else {
                                fprintf(stderr, "Error: invalid command\n");
                                exit(1);
                            }
                        }
                        // <<
                        /*else if (IOtable.loc_oo == i) {
                            
                        }*/
                    }

                    // Normal Programs should finish here
                    int err = execv(cmdFullLine, args);
                    
                    /*printf("cmdfull: %s\n", cmdFullLine);
                    for (int i = 0; i < (int) (sizeof(args)/sizeof(args[0])); i++) {
                        printf("args[%d]: %s\n", i, args[i]);
                    }*/

                    if (err == -1) {
                        // Errors occur
                        fprintf(stderr, "Error: invalid program\n");
                    }
                    // free(cmdOriginal);

                    exit(1);
                    
                }
            } 
                
            // !!! [RETURN FROM] PARENT PROCESS !!!
            // Parent process (wait for all child processes)
            // close file descriptors first (To avoid stuck in the program!)
            if (pipeNum > 0) {
                for (int i = 0; i < pipeNum; i++) {
                    close(mulPipe[i][0]);
                    close(mulPipe[i][1]);
                }
            }

            pid_t returnP;
            if (pipeNum == 0) {
                returnP = waitpid(-1, &returnStat, WUNTRACED);
            }
            else if (pipeNum > 0) {
                int nullStat;
                for (int i = 0; i <= pipeNum; i++) {
                    waitpid(-1, &nullStat, 0); // do not need to use WUNTRACED (no stop signal here)
                }
            }
            
            // Understand what status the child process return
            // store the information required for jobs command
            if (WIFSTOPPED(returnStat)) {
                // trackSuspendIDs start from 1, but trackSuspendJobs start from 0
                int findPidLoc = -1;                    
                for (int j = 0; j < trackSuspendIDs - 1; j++) {
                    if (trackSuspendJobs[j].job_pid == returnP) {
                        findPidLoc = j;
                        break;
                    }
                }

                if (findPidLoc == -1) {
                    int aid = trackSuspendIDs-1;
                    strcpy(trackSuspendJobs[aid].line , trackSuspendLine);
                    trackSuspendJobs[aid].job_mid = trackSuspendIDs;
                    trackSuspendJobs[aid].job_pid = returnP;

                    trackSuspendIDs++;
                }
                
                //printf("The child is suspended. The job is stored in jobs ID %d\n", trackSuspendIDs);
                //continue;
            }

            //printf("out main\n");

            // clean up the memory
            
            // correct code before
            //free(cmdUserbin);
                    
            
        }
        else {
            // !!! FOR PARENT PROCESS !!!
            // Four Customed Commands are placed here! (Previously place in child process but lots of errors exist!)
            // check current directory after using cd
            /*
                Commands:
                1. cd <dir>
                2. jobs
                3. fg <index>
                4. exit
            */

            int listSize = 0;
            while (argList[0][listSize]) {
                listSize++;
            }

            if (strcmp(argList[0][0], "cd") == 0) {
                // rootPath (variable) : mean that the previous path dir is in
                
                // detect errors
                if (listSize != 2) {
                    fprintf(stderr, "Error: invalid command\n");
                    continue;
                }
                int err = chdir(argList[0][1]);
                if (err == -1) {
                    fprintf(stderr, "Error: invalid directory\n");
                    continue;
                }
                
                currPath = getcwd(pathBuff, 500);

                baseName = extract_basename(currPath); // get the baseName

                currPath = getcwd(pathBuff, 500); // correct currPath !
            }
            else if (strcmp(argList[0][0], "exit") == 0) {

                if (trackSuspendIDs > 1) {
                    fprintf(stderr, "Error: there are suspended jobs\n");
                    //fflush(stdout);
                    continue;
                }
                

                if (listSize != 1) {
                    fprintf(stderr, "Error: invalid command\n");
                    //fflush(stdout);
                    continue;
                }
                else {
                    //fflush(stdout);
                    break;
                }
            }
            else if (strcmp(argList[0][0], "jobs") == 0) {
                // get current pid
                // detect errors
                if (listSize != 1) {
                    fprintf(stderr, "Error: invalid command\n");
                    continue;
                }
                
                // no jobs need to be printed
                if (trackSuspendIDs == 1) {
                    continue;
                }

                int trackjb = 0;
                while (trackjb < trackSuspendIDs - 1) {
                    printf("%s\n", trackSuspendJobs[trackjb].line);
                    trackjb++;
                }

                continue;
            }
            else if (strcmp(argList[0][0], "fg") == 0) {
                // detect errors
                if (listSize != 2) {
                    fprintf(stderr, "Error: invalid command\n");
                    continue;
                }
                
                // turn argsList[1] to integer
                int fgID = atoi(argList[0][1]);

                // if job id does not exist
                if (trackSuspendIDs <= fgID) {
                    fprintf(stderr, "Error: invalid job\n");
                    continue;
                }

                pid_t resumePID = trackSuspendJobs[fgID - 1].job_pid;
                

                if (trackSuspendIDs >= fgID + 1) {
                    // enter [fgID - 1]
                    // resume the previous process
                    int susStat;
                    kill(resumePID, SIGCONT);
                    

                    // wait resumed child process
                    waitpid(resumePID, &susStat, WUNTRACED);
                    
                    // if the resumed process is exited

                    if ((!WIFSTOPPED(susStat)) && fgID != -1 && trackSuspendIDs >= fgID) {
                        // enter [fgID - 1]
                        //printf("retrieve pid: %d\n", trackSuspendIDs);

                        // clean the part in the trackSuspendJobs
                        strcpy(trackSuspendJobs[fgID - 1].line, "\0");
                        trackSuspendJobs[fgID - 1].job_mid = -1;
                        trackSuspendJobs[fgID - 1].job_pid = -1;
                        
                        
                        // 
                        trackSuspendIDs--;
                        //printf("trackSuspendIDs: %d\n", trackSuspendIDs);
                        //printf("fgID: %d\n", fgID);

                        // rearrange trackSuspendJobs
                        if (trackSuspendIDs > 1) {
                            for (int i = fgID - 1; i <= trackSuspendIDs - 2; i++) {
                                //printf("fgID: %d\n", fgID);
                                // reconstruct the string (parse & concatenate)
                                char* spc = strchr(trackSuspendJobs[i+1].line, ' ');
                                int loc = spc - trackSuspendJobs[i+1].line;
                                char sub[500];
                                int last_loc = strlen(trackSuspendJobs[i+1].line) - loc + 1;
                                for (int j = 0; j < ((int) strlen(trackSuspendJobs[i+1].line) - loc); j++) {
                                    sub[j] = trackSuspendJobs[i+1].line[j+loc+1];
                                }
                                sub[last_loc] = '\0';

                                char* newJobLine = create_jobs_command(sub, i + 1);

                                strcpy(trackSuspendJobs[i].line, newJobLine);
                                trackSuspendJobs[i].job_mid = trackSuspendJobs[i+1].job_mid;
                                trackSuspendJobs[i].job_pid = trackSuspendJobs[i+1].job_pid;
                            }
                        }

                        strcpy(trackSuspendJobs[trackSuspendIDs - 1].line, "\0");
                        trackSuspendJobs[trackSuspendIDs - 1].job_mid = -1;
                        trackSuspendJobs[trackSuspendIDs - 1].job_pid = -1;
                
                    }
                    else if ((WIFSTOPPED(susStat))) {
                        continue;
                    }
                }
                
                else {
                    // fgID does not exist!
                    exit(1);
                }
            }
        }
        
        // free argList[0....n] here
        for (int i = 0; i < 10; i++) {
            /*for (int j = 0; j < 50; j++) {
                free(argList[i][j]);
            }*/
            free(argList[i]);
        }

        // free argList here
        //free(allTokens);
        //free(argList);
        free(allTokens);
    }

    // free all the allocated memories
    //free(cmdLine);
    //free(baseName);
    //free(currPath);

    return 0;
}
