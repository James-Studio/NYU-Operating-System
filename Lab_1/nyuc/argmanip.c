#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

char **manipulate_args(int argc, const char *const *argv, int (*const manip)(int)) {
    // command line input: # ./nyuc Hello, world

    if (argc == 0) {
        return NULL;
    }
    
    // use malloc to copy the augment list
    // create 1 * n (2-dimension vector)
    // be aware of the place that should add 1 (neglect the problem before!)
    char** strs = (char**) malloc ((argc+1) * sizeof(char*)); // string before the function (a copy of the original string)
    char** strsAfter = (char**) malloc ((argc+1) * sizeof(char*)); // string after the function
    // initialize the last 
    strs[argc] = NULL;
    strsAfter[argc] = NULL;
    // initialize strs and strsAfter to prevent memory errors
    /*
    for (int i = 0; i < argc + 1; i++) {
        strs[i] = NULL;
        strsAfter[i] = NULL;
    }*/
    

    // duplicate the original string
    for (int i = 0; i < argc; i++) {
        
        strs[i] = (char*) malloc ((strlen(argv[i]) + 1) * (sizeof(char)));
        
        int argv_len = (int)strlen(argv[i]);
        for (int n = 0; n < argv_len + 1; n++) {
            strs[i][n] = argv[i][n];
        }
    }

    // pass the original string to the function
    for (int i = 0; i < argc; i++) {

        strsAfter[i] = (char*) malloc ((strlen(argv[i]) + 1) * sizeof(char));
        int argv_len = (int)strlen(argv[i]);
        for (int n = 0; n < argv_len + 1; n++) {
            int get_char = manip(strs[i][n]);
            char new_char = (char) get_char;
            strsAfter[i][n] = new_char;
        }
    }

    // free the original string here (strs), retain strsAfter
    for (int i = 0; i < argc; i++) {
        free(strs[i]);
    }
    free(strs);

    return strsAfter;
}

void free_copied_args(char **args, ...) {
    // Reference: The C Programming Language 2nd Edition Textbook, Section 7.3 (for "variable-length argument list")
    va_list arg_list;

    // get_arg points to the first unamed arg after the first parameter "args"
    va_start(arg_list, **args);

    // so free args first
    int count_mem = 0;
    while (args[count_mem] != NULL) {
        count_mem++;
    }
    for (int i = 0; i < count_mem; i++) {
        free(args[i]);
    }
    free(args);

    // free unamed arguments
    // create an infinite loop with "while"
    while (1) {
        int second_count_mem = 0;
        char** get_arg = va_arg(arg_list, char**);
        

        // check if all the arguments are processed!
        if (get_arg == NULL) {
            break;
        }

        // find the size of the memory and deallocate them
        while (get_arg[second_count_mem] != NULL) {
            second_count_mem++;
        }

        for (int i = 0; i < second_count_mem; i++) {
            free(get_arg[i]);
        }
        free(get_arg);
    }
    
    va_end(arg_list); // cleanup the va_list
    
    return;
}