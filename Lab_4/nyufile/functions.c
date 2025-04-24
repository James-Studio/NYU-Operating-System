#include "functions.h"

void print_boot_sector_info(BootEntry* boot_info) {
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
