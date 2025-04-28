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
    size_t boot_sector_size = 90; // like the lecture said
    BootEntry *boot_sector_info;
            
    int diskd = open(diskname, O_RDWR);
    
    void *map_boot = mmap(NULL, boot_sector_size, PROT_READ | PROT_WRITE, MAP_SHARED, diskd, 0);
    
    boot_sector_info = (BootEntry *) map_boot;
    
    // get the end of reserved area
    // fat_off (fat_start): the offset of fat table
    int fat_off = (int) (boot_sector_info->BPB_BytsPerSec * boot_sector_info->BPB_RsvdSecCnt);
    size_t fat_size = (size_t) boot_sector_info->BPB_BytsPerSec * boot_sector_info->BPB_FATSz32 * boot_sector_info->BPB_NumFATs;
    printf("fat_off: %d\n", fat_off);

    // get the first byte of data area
    int data_area_off = fat_off + ((int) fat_size); // equals to (DirEntry *)(mapped_address + 0x5000)
    printf("data_area_off: %d\n", data_area_off);
    
    // get the size of the disk
    struct stat disk_stat;
    int return_ret = fstat(diskd, &disk_stat);
    char *disk_info = mmap(NULL, disk_stat.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, diskd, 0);
    // clus_size: used in "Data Area"
    // size_t clus_size = (size_t) boot_sector_info->BPB_BytsPerSec * boot_sector_info->BPB_SecPerClus;

    // get the start of the fat entry
    int *fat_entry_start = (int *) (disk_info + fat_off);
    int root_cluster = boot_sector_info->BPB_RootClus; // usually 2

    if (mode == 1) {
        //
        print_boot_sector_info(boot_sector_info);
    }
    else if (mode == 2) {
        // print_directory_info
        DirEntry *dir_entry_info = (DirEntry *) (disk_info + data_area_off);
        int next_fat_entry = root_cluster;

        int count_entries = 0;

        while (next_fat_entry < 0x0ffffff8) {

            // get the next entry number: mask with 0x0fffffff
            // address method: long get_next_entry = ((FatEntry *) (disk_info + fat_start + fat_sec_size * get_next_entry))->FAT_NextEntryID & 0x0FFFFFFF;
            int dir_count = ((int) (boot_sector_info->BPB_BytsPerSec * boot_sector_info->BPB_SecPerClus)) / sizeof(DirEntry);
            
            for (int i = 0; i < dir_count; i++) {
                // check the data area for this cluster
                DirEntry *dir_info = (DirEntry *) (dir_entry_info + (next_fat_entry - 2));
                
                // make sure the file is not nothing
                if ((dir_info[i].DIR_Name)[0] != 0x00 && (dir_info[i].DIR_Name)[0] != 0xe5) {
                    count_entries++;
                    print_dir_info(dir_info, i);
                }
                else if ((dir_info[i].DIR_Name)[0] == 0xe5) {
                    continue;
                }
                else {
                    break;
                }
            }

            // index method
            next_fat_entry = fat_entry_start[next_fat_entry] & 0x0fffffff;            
        } 
        printf("Total number of entries = %d\n", count_entries);
    }
    else if (mode == 3) {
        // have filename here
        // Milestone 4: Recover a small file ONLY in the root directory
        // ./nyufile fat32.disk -r HELLO.TXT

        // print_directory_info
        int dir_count = ((int) (boot_sector_info->BPB_BytsPerSec * boot_sector_info->BPB_SecPerClus)) / sizeof(DirEntry);

        DirEntry *dir_entry_info = (DirEntry *) (disk_info + data_area_off);

        DirEntry *dir_info = (DirEntry *) (dir_entry_info + (root_cluster - 2));
        
        bool find_file = false;

        for (int i = 0; i < dir_count; i++) {
            if ((dir_info[i].DIR_Name)[0] != 0x00) {
                if ((dir_info[i].DIR_Name)[0] == 0xe5) {
                    //
                    int filename_id = 1;
                    bool check_res = true;

                    for (int wd = 1; wd < 11; wd++) {
                        if ((dir_info[i].DIR_Name)[wd] == ' ') {
                            continue;
                        }

                        if ((dir_info[i].DIR_Name)[wd] != filename[filename_id]) {
                            check_res = false;
                            break;
                        }

                        filename_id++;
                        if (filename[filename_id] == '.') {
                            filename_id++;
                        }
                    }

                    // find the file and recover it
                    if (check_res == true) {
                        find_file = true;
                        // change the delete file character back to the original character
                        (dir_info[i].DIR_Name)[0] = filename[0];
                        printf("%s: successfully recovered\n", filename);
                        break;
                    }
                }
                else {
                    continue;
                }
            }
            else {
                if (find_file == false) {
                    printf("%s: file not found\n", filename);
                }
                break;
            }
        }
        

    }
    else {
        // fix the warning
        printf("mode_s: %d, %d\n", mode_s, return_ret);
    }

    //
    close(diskd);
}
