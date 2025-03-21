#include "functions.h"

int main(int argc, char **argv) {
  /*  Milestone 1: read files from one or more

    argv[argc-1]: output file, argv[argc-2]: >
    argv[1] ~ argv[argc-3] (argc - 3 may equal to 1 -> smallest situation)

  * Remember: No copying in the data (Slow the program down)
  * 
  * 
  */ 
  
  /*int fd, byteRd;
  char *fullString = (char*) malloc(50 * sizeof(char));
  char rdBuffer[100]; // Not sure how big the size of the data is
  
  // concat the strings together
  // use open(), close() sys calls
  for (int i = 1; i <= (argc-1); i++) {
    fd = open(argv[i], O_RDONLY);
    printf("file: %s\n", argv[i]);
    while (read(fd, rdBuffer, 49)) {
      rdBuffer[49] = '\0';
      printf("string: %s\n", rdBuffer);
      strcat(fullString, rdBuffer);
      fullString = (char*) realloc(fullString, 50 * sizeof(char));
    }
    close(fd);
  }*/
  
  // Open file
  int fd;
  struct stat *sb = (struct stat*) malloc((argc-1)* sizeof(struct stat));
  char **addrList = (char**) malloc((argc-1) * sizeof(char*));

  for (int f = 0; f < argc-1; f++) {
    fd = open(argv[f+1], O_RDONLY);
    // printf("argv: %s\n", argv[f+1]);

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
    addrList[f] = mmap(NULL, sb[f].st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (addrList[f] == MAP_FAILED) {
      fprintf(stderr, "Errors in mmap\n");
      return 1;
    }
    close(fd);
  }

  // redirect the output location
  int new_fd = open(argv[argc-1], O_RDWR | O_CREAT | O_TRUNC);
  dup2(new_fd, 1);

  // do not make copy of the string anymore
  

  

}