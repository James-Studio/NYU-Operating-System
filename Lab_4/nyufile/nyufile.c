#include "functions.h"

int main(int argc, char **argv) {
    int opt; // get arguments in command flags (optarg)
    char filename[100];
    char sha1[200];
    int mode = -1;
    bool mode_s = false;


    /*
        Usage: ./nyufile disk <options>
                -i                     Print the file system information.
                -l                     List the root directory.
                -r filename [-s sha1]  Recover a contiguous file.
                -R filename -s sha1    Recover a possibly non-contiguous file.
    */
    while ((opt = getopt(argc, argv, "r:R:s:il")) != -1) {
        switch(opt) {
            case 'i':
                mode = 1;
                break;
            case 'l':
                mode = 2;
                break;
            case 'r':
                mode = 3;
                strcpy(filename, optarg);
                break;
            case 'R':
                mode = 4;
                strcpy(filename, optarg);
                break;
            case 's':
                mode_s = true;
                strcpy(sha1, optarg);
                break;
            case '?':
                // if error, print the above usage information verbatim and exit
                for (int i = 0; i < argc; i++) {
                    printf("%s\n", argv[i]);
                    if (i < argc - 1) {
                        printf(" ");
                    }
                }
                exit(1);
        }
    }

    //


    
}
