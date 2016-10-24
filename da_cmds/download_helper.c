#include <file_util.h>
#include <mtkparse.h>
#include "inc/da_cmds.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int download_download_agent(tty_usb_handle *h, char *da_path, uint16_t chip_code)
{
    struct mtk_da da = mtk_da_parse(da_path);
    struct mtk_da_file_header da_header = mtk_find_download_agent(&da, chip_code);
    if (da_header.magic[0] == 0x0 && da_header.magic[1] == 0x0) {
        printf("Can't find files for %04x in %s.\n", chip_code, da_path);
        return 1;
    }
    int da_swsec = open(da_path, O_RDONLY);

    char *pld_data = malloc(da_header.pld.code_size);
    lseek(da_swsec, da_header.pld.addr, SEEK_SET);
    read(da_swsec, pld_data, da_header.pld.code_size);

    // TODO: check if there is a signature
    char *pld_sign = malloc(da_header.pld.sign_len);
    read(da_swsec, pld_sign, da_header.pld.sign_len);

    int r = send_da(h, da_header.pld.dest, pld_data, da_header.pld.code_size, pld_sign, da_header.pld.sign_len);

    free(pld_data);
    free(pld_sign);

    if(r)
    {
        printf("ERROR: send_da(%d)\n", r);
        return 1;
    }
    printf("Preloader sent\n");

    r = jump_da(h, da_header.pld.dest);
    if(r)
    {
        printf("ERROR: jump_da(%d)\n", r);
        return 1;
    }

    if (tty_usb_r8(h) == 0xC0)
        get_emmc_nand_info(h);

    pld_data = malloc(da_header.da.code_size);
    lseek(da_swsec, da_header.da.addr, SEEK_SET);
    read(da_swsec, pld_data, da_header.da.code_size);

    // TODO: check if there is a signature
    pld_sign = malloc(da_header.da.sign_len);
    read(da_swsec, pld_sign, da_header.da.sign_len);

    send_da_part_2(h, da_header.da.dest, pld_data, da_header.da.code_size, pld_sign, da_header.da.sign_len);
    printf("Download agent sent\n");

    free(pld_data);
    free(pld_sign);
    close(da_swsec);

    return 0;
}

