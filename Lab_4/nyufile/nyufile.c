#include "functions.h"

int main(int argc, char **argv) {
    int opt; // get arguments in command flags (optarg)
    char diskname[100];
    char filename[100];
    char sha1[200];
    int mode = -1;
    bool mode_s = false;

    // get the name of the disk
    strcpy(diskname, argv[1]);
    
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

    // Boot sector is the first 512 bytes of the FS
    size_t boot_sector_size = 512;
        
    int diskd = open(diskname, O_RDWR);
    
    BootEntry *boot_sector_info = (BootEntry *) malloc(sizeof(BootEntry));
    void *map_boot = mmap(NULL, boot_sector_size, PROT_READ | PROT_WRITE, MAP_SHARED, diskd, 0);
    boot_sector_info = (BootEntry *) map_boot;
    
    //
    printf("mode: %d, modes: %d\n", mode, mode_s);
    
    print_boot_sector_info(boot_sector_info);

    


    
}
