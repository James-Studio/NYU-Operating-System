#include "functions.h"

void print_boot_sector_info(BootEntry *boot_info) {
    /*
        Output for flag -i:
            Number of FATs = 2
            Number of bytes per sector = 512
            Number of sectors per cluster = 1
            Number of reserved sectors = 32
    */
    printf("Number of FATs: %d\n", boot_info->BPB_NumFATs);
    printf("Number of bytes per sector: %d\n", boot_info->BPB_BytsPerSec);
    printf("Number of sectors per cluster: %d\n", boot_info->BPB_SecPerClus);
    printf("Number of reserved sectors: %d\n", boot_info->BPB_RsvdSecCnt);
    return;
}

void print_dir_info(DirEntry *dir_info, int dir_id) {
    /*  
        Filename. Similar to /bin/ls -p, if the entry is a directory, you should append a / indicator.
        File size if the entry is a file (not a directory).
        Starting cluster if the entry is not an empty file.
        ---------------------------------------------------------------
        HELLO.TXT (size = 14, starting cluster = 3)
        DIR/ (starting cluster = 4)
        EMPTY (size = 0)
        Total number of entries = 3
    */
    
    // check whether the file is deleted file
    int start_cluster =  dir_info[dir_id].DIR_FstClusHI << 16 | dir_info[dir_id].DIR_FstClusLO;
    
    // Case 1: File, but not Empty
    if (dir_info[dir_id].DIR_FileSize != 0) {
        print_file(dir_info[dir_id].DIR_Name);
        printf(" (size = %d, starting cluster = %d)\n", dir_info[dir_id].DIR_FileSize, start_cluster);
    }
    // Case 2: Directory
    else if ((dir_info[dir_id].DIR_Attr & 0x10) > 0) {
        // print till empty
        for (int i = 0; i < 11; i++) {
            if ((dir_info[dir_id].DIR_Name)[i] != ' ') {
                printf("%c", (dir_info[dir_id].DIR_Name)[i]);
            }
        }
        printf("/ (starting cluster = %d)\n", start_cluster);
    }
    // Case 3: File, but Empty
    else {
        // print till empty
        for (int i = 0; i < 11; i++) {
            if ((dir_info[dir_id].DIR_Name)[i] != ' ') {
                printf("%c", (dir_info[dir_id].DIR_Name)[i]);
            }
        }
        printf(" (size = %d)\n", dir_info[dir_id].DIR_FileSize);
    }

    return;
}

void print_file(unsigned char *dir_name) {
    // print file name file[0] ~ file[7]
    for (int i = 0; i < 8; i++) {
        if (dir_name[i] != ' ') {
            printf("%c", dir_name[i]);
        }
    }
    
    if (dir_name[8] != ' ') {
        printf(".");
    }

    // print extension name file[8] ~ file[10]
    for (int i = 8; i < 11; i++) {
        if (dir_name[i] != ' ') {
            printf("%c", dir_name[i]);
        }
    } 
}