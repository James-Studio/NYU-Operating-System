/*
    Reference:
    StackOverflow: https://stackoverflow.com/questions/3969047/is-there-a-standard-way-of-representing-an-sha1-hash-as-a-c-string-and-how-do-i
*/

#include "functions.h"

int main(int argc, char **argv) {
    int opt; // get arguments in command flags (optarg)
    char diskname[100];
    char *filename = (char *) malloc(100 * sizeof(char));
    char target_sha_hash[SHA_DIGEST_LENGTH * 2 + 1];
    int mode = -1;
    bool mode_s = false;


    // deal with some exceptions
    if (argc == 1 || strcmp(argv[1], "-i") == 0) {
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
                strcpy(target_sha_hash, optarg);
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

    // get the first byte of data area
    int data_area_off = fat_off + ((int) fat_size); // equals to (DirEntry *)(mapped_address + 0x5000)
    
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
        int next_fat_entry = root_cluster;

        int count_entries = 0;
        int cluster_bytes = (int) (boot_sector_info->BPB_BytsPerSec * boot_sector_info->BPB_SecPerClus);
        int dir_count = cluster_bytes / sizeof(DirEntry);

        while (next_fat_entry < 0x0ffffff8) {            
            char* dir_loc = disk_info + data_area_off + (next_fat_entry - 2) * cluster_bytes;
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
            // printf("fat_entry_start[next_fat_entry]: %d\n", fat_entry_start[next_fat_entry]);
            next_fat_entry = fat_entry_start[next_fat_entry];
        }
        printf("Total number of entries = %d\n", count_entries);
    }
    else if (mode == 3) {
        // have filename here
        // ./nyufile fat32.disk -r HELLO.TXT
        
        // 

        int next_fat_entry = root_cluster;

        int cluster_size = (int) (boot_sector_info->BPB_BytsPerSec * boot_sector_info->BPB_SecPerClus);
        int dir_count = cluster_size / sizeof(DirEntry);

        //
        int num_contiguous = 0;
        int num_ambiguous = 0, get_del_fat = -1, record_del_cluster = -1; 
        bool hash_found = false;

        while (next_fat_entry < 0x0ffffff8 && hash_found == false) {            
            //
            char* dir_loc = disk_info + data_area_off + (next_fat_entry - 2) * cluster_size;
            DirEntry *dir_info = (DirEntry *) (dir_loc);
            
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
                            // if mode_s == true (SHA-1 is provided)

                            // #define SHA_DIGEST_LENGTH 20
                            if (mode_s == true) {
                                // turn the name back
                                unsigned char new_sha_temp[SHA_DIGEST_LENGTH];
                                char new_sha_hash[SHA_DIGEST_LENGTH * 2 + 1];
                                int get_start_clus = (dir_info[i].DIR_FstClusHI << 16 | dir_info[i].DIR_FstClusLO);
                                
                                
                                // check whether SHA-1 is the same
                                SHA1(((unsigned char *) (dir_loc + (get_start_clus - 2) * cluster_size)), ((size_t) dir_info[i].DIR_FileSize), new_sha_temp);
                                
                                for (int s = 0; s < SHA_DIGEST_LENGTH; s++) {
                                    sprintf(new_sha_hash + s*2, "%02x", new_sha_temp[s]);
                                }

                                if (strcmp(((char *) new_sha_hash), ((char *) target_sha_hash)) == 0) {
                                    num_contiguous = (dir_info[i].DIR_FileSize / cluster_size) + 1;

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
                                get_del_fat = next_fat_entry;
                                record_del_cluster = i;
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
                next_fat_entry = fat_entry_start[next_fat_entry];    
            }
            else {
                break;
            }
        }

        // check ambiguity
        if (mode_s == false) {
            if (num_ambiguous == 1) { 
                // change the delete file character back to the original character
                char* dir_loc = disk_info + data_area_off + (get_del_fat - 2) * cluster_size;
                DirEntry *dir_info = (DirEntry *) (dir_loc);
    
                (dir_info[record_del_cluster].DIR_Name)[0] = filename[0];
    
                next_fat_entry = (dir_info[record_del_cluster].DIR_FstClusHI << 16) | dir_info[record_del_cluster].DIR_FstClusLO;
                num_contiguous = (dir_info[record_del_cluster].DIR_FileSize / cluster_size) + 1;
    
    
                if (num_contiguous <= 1) {
                    fat_entry_start[next_fat_entry] = 0x0ffffff8;
                    printf("%s: successfully recovered\n", filename);
                }
                else {
                    for (int f = 1; f < num_contiguous; f++) {
                        fat_entry_start[next_fat_entry + f - 1] = next_fat_entry + f;                            
                    }
                    fat_entry_start[next_fat_entry + num_contiguous - 1] = 0x0ffffff8;
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
        exit(0);

        // have filename here
        // ./nyufile fat32.disk -r HELLO.TXT

        //  correct code
        
        int next_fat_entry = root_cluster;

        int cluster_size = (int) (boot_sector_info->BPB_BytsPerSec * boot_sector_info->BPB_SecPerClus);
        int dir_count = cluster_size / sizeof(DirEntry);

        //
        int num_contiguous = 0;
        int num_ambiguous = 0, get_del_fat = -1, record_del_cluster = -1; 

        while (next_fat_entry < 0x0ffffff8) {
            //
            char* dir_loc = disk_info + data_area_off + (next_fat_entry - 2) * cluster_size;
            DirEntry *dir_info = (DirEntry *) (dir_loc);
            
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
                            num_ambiguous++;
                            get_del_fat = next_fat_entry;
                            record_del_cluster = i;
                            check_res = false;
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
            next_fat_entry = fat_entry_start[next_fat_entry];    
        }

        //
        if (num_ambiguous == 1) {
            // change the delete file character back to the original character
            char* dir_loc = disk_info + data_area_off + (get_del_fat - 2) * cluster_size;
            DirEntry *dir_info = (DirEntry *) (dir_loc);

            (dir_info[record_del_cluster].DIR_Name)[0] = filename[0];

            next_fat_entry = (dir_info[record_del_cluster].DIR_FstClusHI << 16) | dir_info[record_del_cluster].DIR_FstClusLO;
            num_contiguous = (dir_info[record_del_cluster].DIR_FileSize / cluster_size) + 1;

            if (num_contiguous <= 1) {
                fat_entry_start[next_fat_entry] = 0x0ffffff8;
                printf("%s: successfully recovered\n", filename);
            }
            else {
                for (int f = 1; f < num_contiguous; f++) {
                    fat_entry_start[next_fat_entry + f - 1] = next_fat_entry + f;                            
                }
                fat_entry_start[next_fat_entry + num_contiguous - 1] = 0x0ffffff8;
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
    else {
        // fix the warning
        printf("mode_s: %d, %d\n", mode_s, return_ret);
    }

    //
    close(diskd);
}
