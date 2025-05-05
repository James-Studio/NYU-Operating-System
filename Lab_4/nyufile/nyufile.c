/*
    < Reference >

    StackOverflow: 
    1. https://stackoverflow.com/questions/3969047/is-there-a-standard-way-of-representing-an-sha1-hash-as-a-c-string-and-how-do-i
    2. https://stackoverflow.com/questions/918676/generate-sha-hash-in-c-using-openssl-library

    GeeksforGeeks:
    1. https://www.geeksforgeeks.org/left-shift-right-shift-operators-c-cpp/
*/

#include "functions.h"

int main(int argc, char **argv) {
    int opt; // get arguments in command flags (optarg)
    char diskname[100];
    char *filename = (char *) malloc(100 * sizeof(char));
    char target_sha_hash[41];
    int mode = -1;
    bool mode_s = false;

    
    // deal with some exceptions
    if (argc < 3 || strcmp(argv[1], "-i") == 0 || (argv[2][0] == '-' && argv[2][1] == '\0')) {
        print_flag_info();
        exit(0); // exit normally
    }

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
                if (mode > 0) {
                    print_flag_info();
                    exit(0); // exit normally
                }
                mode = 1;
                break;
            case 'l':
                if (mode > 0) {
                    print_flag_info();
                    exit(0); // exit normally
                }
                mode = 2;
                break;
            case 'r':
                if (mode > 0 || optarg == NULL || optarg[0] == '-') {
                    print_flag_info();
                    exit(0); // exit normally
                }
                mode = 3;
                strcpy(filename, optarg);
                break;
            case 'R':
                if (mode > 0 || optarg == NULL || optarg[0] == '-') {
                    print_flag_info();
                    exit(0); // exit normally
                }
                mode = 4;
                strcpy(filename, optarg);
                break;
            case 's':
                if (mode_s == true || mode < 3) {
                    print_flag_info();
                    exit(0); // exit normally
                }
                mode_s = true;
                strcpy(target_sha_hash, optarg);
                break;
            case '?':
                // if error, print the above usage information verbatim and exit
                print_flag_info();
                exit(0); // exit normally
        }
    }
    
    

    // Boot sector is the first 512 bytes of the FS
    // In the lab description, it is 0 - 89 Bytes (90 Bytes in total)
    size_t boot_sector_size = 90;
    BootEntry *boot_sector_info;
    
    /*
        Using mmap() to access the disk image is more convenient than read() or fread(). 
        You may need to open the disk image with O_RDWR and map it with PROT_READ | PROT_WRITE and MAP_SHARED in order to update the underlying file. 
        Once you have mapped your disk image, you can cast any address to the FAT32 data structure type, 
        such as (DirEntry *)(mapped_address + 0x5000). 
        You can also cast the FAT to int[] for easy access.
    */
    int diskd = open(diskname, O_RDWR);
    
    void *map_boot = mmap(NULL, boot_sector_size, PROT_READ | PROT_WRITE, MAP_SHARED, diskd, 0);
    
    boot_sector_info = (BootEntry *) map_boot;
    
    // get the end of reserved area
    // fat_start: the starting byte of fat table
    int fat_start = (int) (boot_sector_info->BPB_BytsPerSec * boot_sector_info->BPB_RsvdSecCnt);
    
    // size_t fat_size = (size_t) (boot_sector_info->BPB_BytsPerSec * boot_sector_info->BPB_NumFATs * boot_sector_info->BPB_FATSz32);
    size_t fat_all_size = (size_t) (boot_sector_info->BPB_BytsPerSec * boot_sector_info->BPB_NumFATs * boot_sector_info->BPB_FATSz32);
    //size_t fat_single_size = (size_t) (boot_sector_info->BPB_BytsPerSec * boot_sector_info->BPB_FATSz32);
    //int fat_table_num = boot_sector_info->BPB_NumFATs;
    

    // get the first byte of data area (the start of the data_area)
    int data_area_start = fat_start + ((int) fat_all_size);

    // get the size of the disk
    struct stat disk_stat;
    int return_ret = fstat(diskd, &disk_stat);
    char *disk_info = mmap(NULL, disk_stat.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, diskd, 0);


    // clus_size: used in "Data Area"

    // get the start of the fat entry
    int *fat_entry_start = (int *) (disk_info + fat_start);
    int root_dir_clus = boot_sector_info->BPB_RootClus; // usually 2

    // place outside different modes
    int clus_size = (int) (boot_sector_info->BPB_BytsPerSec * boot_sector_info->BPB_SecPerClus); 

    if (mode == 1) {
        //
        print_boot_sector_info(boot_sector_info);
    }
    else if (mode == 2) {
        // print_directory_info
        int next_dir_ent_clus = root_dir_clus;

        // record the total number of entries
        int count_entries = 0;
        
        int dir_count = clus_size / sizeof(DirEntry);

        // end of the clus: 0x0ffffff8
        while (next_dir_ent_clus < 0x0ffffff8) {            
            // because the root directory cluster starts from 2
            char* dir_loc = disk_info + data_area_start + (next_dir_ent_clus - 2) * clus_size;

            // check the data area for this cluster
            DirEntry *dir_info = (DirEntry *) (dir_loc);
            
            for (int d = 0; d < dir_count; d++) {
                // make sure the file is not nothing
                if ((dir_info[d].DIR_Name)[0] != 0x00 && (dir_info[d].DIR_Name)[0] != 0xe5) {
                    count_entries++;
                    print_dir_info(dir_info, d);
                }
                else if ((dir_info[d].DIR_Name)[0] == 0xe5) {
                    continue;
                }
                else {
                    break;
                }
            }

            // index method
            // printf("fat_entry_start[next_dir_ent_clus]: %d\n", fat_entry_start[next_dir_ent_clus]);
            next_dir_ent_clus = fat_entry_start[next_dir_ent_clus];
        }
        printf("Total number of entries = %d\n", count_entries);
    }
    else if (mode == 3) {
        // have filename here
        // ./nyufile fat32.disk -r HELLO.TXT

        int next_dir_ent_clus = root_dir_clus;

        int dir_count = clus_size / sizeof(DirEntry);

        //
        int num_contiguous = 0;
        int num_ambiguous = 0, del_dir_ent_clus = -1, record_id_del_clus = -1; 
        bool hash_found = false;

        // if hash is found, break the while loop
        while (next_dir_ent_clus < 0x0ffffff8 && hash_found == false) {            
            // same as mode 2
            char* dir_loc = disk_info + data_area_start + (next_dir_ent_clus - 2) * clus_size;
            DirEntry *dir_info = (DirEntry *) (dir_loc);
            
            // logic same as mode 2
            for (int i = 0; i < dir_count; i++) {
                if ((dir_info[i].DIR_Name)[0] != 0x00) {
                    if ((dir_info[i].DIR_Name)[0] == 0xe5) {
                        //
                        int filename_id = 1;
                        bool check_res = true;

                        // for the deleted file, if the characters except for the first one
                        // are all the same, mark that as potential directory to recover
                        // !!! if no hash is provided !!!
                        for (int wrd = 1; wrd < 11; wrd++) {
                            if ((dir_info[i].DIR_Name)[wrd] == ' ') {
                                continue;
                            }
    
                            if ((dir_info[i].DIR_Name)[wrd] != filename[filename_id]) {
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
                            // if mode_s == true (SHA-1 is provided)

                            // # define SHA_DIGEST_LENGTH 20
                            if (mode_s == true) {
                                // turn the name back
                                unsigned char new_sha_temp[SHA_DIGEST_LENGTH];
                                char new_sha_hash[41];
                                unsigned short high_b = dir_info[i].DIR_FstClusHI;
                                unsigned short low_b = dir_info[i].DIR_FstClusLO;

                                int get_start_clus = (high_b << 16 | low_b);
                                
                                
                                // check whether SHA-1 is the same
                                // unsigned char *SHA1(const unsigned char *d, size_t n, unsigned char *md);
                                SHA1(((unsigned char *) (dir_loc + (get_start_clus - 2) * clus_size)), ((size_t) dir_info[i].DIR_FileSize), new_sha_temp);
                                
                                /* reference from stack overflow */
                                for (int s = 0; s < SHA_DIGEST_LENGTH; s++) {
                                    sprintf(new_sha_hash + s*2, "%02x", new_sha_temp[s]);
                                }

                                //
                                if (strcmp(((char *) new_sha_hash), ((char *) target_sha_hash)) == 0) {
                                    num_contiguous = (dir_info[i].DIR_FileSize / clus_size) + 1;

                                    if (num_contiguous <= 1) {
                                        (dir_info[i].DIR_Name)[0] = filename[0];
                                        fat_entry_start[get_start_clus] = 0x0ffffff8;
                                        hash_found = true;
                                    }
                                    else {
                                        (dir_info[i].DIR_Name)[0] = filename[0];
                                        for (int f = 1; f < num_contiguous; f++) {
                                            fat_entry_start[get_start_clus + f - 1] = get_start_clus + f;                            
                                        }
                                        fat_entry_start[get_start_clus + num_contiguous - 1] = 0x0ffffff8;
                                        hash_found = true;
                                    }

                                    printf("%s: successfully recovered with SHA-1\n", filename);
                                    break;
                                }
                            }
                            else {
                                num_ambiguous++;
                                del_dir_ent_clus = next_dir_ent_clus;
                                record_id_del_clus = i;
                                check_res = false;
                            }
                        }
                    }
                    else {
                        continue;
                    }
                }
                else {
                    break;
                }
            }

            if (hash_found == false) {
                next_dir_ent_clus = fat_entry_start[next_dir_ent_clus];    
            }
            else {
                break;
            }
        }

        // check ambiguity
        if (mode_s == false) {
            if (num_ambiguous == 1) {
                char* dir_loc = disk_info + data_area_start + (del_dir_ent_clus - 2) * clus_size;
                DirEntry *dir_info = (DirEntry *) (dir_loc);

                unsigned short high_b = dir_info[record_id_del_clus].DIR_FstClusHI;
                unsigned short low_b = dir_info[record_id_del_clus].DIR_FstClusLO;
                
                
                // change the delete file character back to the original character
                (dir_info[record_id_del_clus].DIR_Name)[0] = filename[0];
    
                next_dir_ent_clus = (high_b << 16) | low_b;
                num_contiguous = (dir_info[record_id_del_clus].DIR_FileSize / clus_size) + 1;
    
                // check if the file is stored in multiple clusters
                if (num_contiguous <= 1) {
                    fat_entry_start[next_dir_ent_clus] = 0x0ffffff8;
                    printf("%s: successfully recovered\n", filename);
                }
                else {
                    for (int f = 1; f < num_contiguous; f++) {
                        fat_entry_start[next_dir_ent_clus + f - 1] = next_dir_ent_clus + f;                            
                    }
                    fat_entry_start[next_dir_ent_clus + num_contiguous - 1] = 0x0ffffff8;
                    printf("%s: successfully recovered\n", filename);
                   
                }
            }
            else if (num_ambiguous > 1) {
                printf("%s: multiple candidates found\n", filename);
            }
            else {
                printf("%s: file not found\n", filename);
            }
        }

        if (mode_s == true && hash_found == false) {
            printf("%s: file not found\n", filename);
        }
    }
    else if (mode == 4) {
        //
        printf("Redundant return_ret: %d\n", return_ret);
        printf("Do not build the function in the mode\n");
        exit(0);
    }
    else {
        // print error modes
        fprintf(stderr, "The mode does not exist.\n");
    }

    // free the memories
    free(filename);
    close(diskd);
}
