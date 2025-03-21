#include "functions.h"

int main(int argc, char **argv) {
  /*  Milestone 1: read files from one or more

    argv[argc-1]: output file, argv[argc-2]: >
    argv[1] ~ argv[argc-3] (argc - 3 may equal to 1 -> smallest situation)

  * Remember: No copying in the data (Slow the program down)
  * 
  * 
  */ 

  // Open file
  int fd;
  struct stat *sb = (struct stat*) malloc((argc-2)* sizeof(struct stat));
  char **addrList = (char**) malloc((argc-2) * sizeof(char*));

  for (int f = 0; f < (argc-2); f++) {
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
    addrList[f] = mmap(NULL, sb[f].st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (addrList[f] == MAP_FAILED) {
      fprintf(stderr, "Errors in mmap\n");
      return 1;
    }
    close(fd);
  }

  /* -------------------------------------------------------------------------------------------------------------- */

  // do not make copy of the string anymore
  // build encoding algorithm
  
  int countCons = 0, countEncode = 0;
  char currentChar;
  char *encodeStr = (char*) malloc (500 * sizeof(char));

  for (int strCount = 0; strCount < (argc - 2); strCount++) {
    for (int i = 0; i < (int) strlen(addrList[strCount]); i++) {
      // Case 1: Change to the new character && No count
      if (countCons == 0) {
        currentChar = addrList[strCount][i];
        countCons++;
      }
      // Case 2: Change to the new character && Have count 
      // Add characters in the encoded string
      else if (currentChar != addrList[strCount][i]) {
        // Assign the char and number in the string
        encodeStr[countEncode] = currentChar;
        encodeStr[countEncode+1] = countCons;
        countEncode += 2;

        // Change to the new character
        currentChar = addrList[strCount][i];
        countCons = 1;
      }
      // Case 3: Remain the previous character
      else {
        countCons++;
      }
    }
  }

  // Assign the last character and number to the encoded string
  encodeStr[countEncode] = currentChar;
  encodeStr[countEncode+1] = countCons;
  // Still update the recording value
  countCons = 0;
  countEncode += 2;

  // Write data to the STDOUT
  FILE *encodeFilePtr = fopen(argv[argc-1], "wb");

  // Use countEncode as the size of the string written into the encoded file
  fwrite(encodeStr, sizeof(char), countEncode, encodeFilePtr);
  fclose(encodeFilePtr);

  /* ----------------------------------------------------------------------- */
  

}
