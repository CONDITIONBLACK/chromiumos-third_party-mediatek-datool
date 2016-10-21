#include <file_util.h>
#include <mtkparse.h>
#include "inc/da_cmds.h"
#include <stdio.h>
#include <string.h>

enum CHIP_ID {
    CHIP_ID_DEFAULT,
    CHIP_ID_MT8135,
    CHIP_ID_MT8127,
    CHIP_ID_MT6595,
    CHIP_ID_MT8173,
};

typedef struct {
    enum CHIP_ID chip;
    uint32_t pl_load_addr;
    uint32_t lk_wrapper_addr;
} CHIP_DATA;

CHIP_DATA chip_tbl[] = {
    {CHIP_ID_DEFAULT, 0x00201000, 0x40001000},
    {CHIP_ID_MT8135,  0x12001000, 0x80001000},
    {CHIP_ID_MT8127,  0x00201000, 0x80001000},
    {CHIP_ID_MT6595,  0x00201000, 0x40001000},
    {CHIP_ID_MT8173,  0x000C1000, 0x40001000},
};

static CHIP_DATA* get_chip_data(uint16_t chip)
{
    if(chip == 0x8135) return &(chip_tbl[1]);
    if(chip == 0x8127) return &(chip_tbl[2]);
    if(chip == 0x6595) return &(chip_tbl[3]);
    if((chip == 0x8172) || (chip == 0x8176)) return &(chip_tbl[4]);
    return &(chip_tbl[0]);
}

int download_download_agent(tty_usb_handle *h, int is_brom, char *auth_path, char *pl_path,
        char *lk_path, uint16_t chip_code)
{
    CHIP_DATA* chip = get_chip_data(chip_code);
    int r;
    void *lk=NULL, *sig_lk=NULL;
    size_t len_lk, len_sig_lk;
    char lk_sig_path[4096];

    if(is_brom)
    {
        uint32_t cfg;
        void *pl=NULL, *sig_pl=NULL, *strip_pl=NULL;
        size_t len_pl, len_sig_pl, len_strip_pl;

        r = get_target_config(h, &cfg);
        if(r)
        {
            printf("ERROR: get_target_config(%d)\n", r);
            return 1;
        }
        if(TGT_CFG_DAA_EN & cfg)
        {
            void *auth;
            size_t len_auth;

            if((TGT_CFG_SBC_EN & cfg) == 0)
            {
                printf("ERROR: daa=1, sbc=0\n");
                return 1;
            }

            if(auth_path==NULL)
            {
                printf("ERROR: no auth file\n");
                return 1;
            }

            load_binary(auth_path, &auth, &len_auth);
            r = send_auth(h, auth, len_auth);
            free(auth);
            if(r)
            {
                printf("ERROR: send_auth(%d)\n", r);
                return 1;
            }
            printf("send %s\n", auth_path);
        }

        load_binary(pl_path, &pl, &len_pl);
        strip_pl_hdr(pl, len_pl, &strip_pl, &len_strip_pl);
        if(TGT_CFG_DAA_EN & cfg)
        {
            char sig_path[4096];
            strcpy(sig_path, pl_path);
            strcat(sig_path, ".sign");
            load_binary(sig_path, &sig_pl, &len_sig_pl);
        }
        else
            len_sig_pl = 0;
        r = send_da(h, chip->pl_load_addr, strip_pl, len_strip_pl, sig_pl, len_sig_pl);
        if(pl) free(pl);
        if(sig_pl) free(sig_pl);
        if(r)
        {
            printf("ERROR: send_da(%d)\n", r);
            return 1;
        }
        printf("send %s\n", pl_path);

        r = jump_da(h, chip->pl_load_addr);
        if(r)
        {
            printf("ERROR: jump_da(%d)\n", r);
            return 1;
        }

        tty_usb_close(h);
        h = NULL;

        // open preloader tty
        h = tty_usb_open_pl();

        r = start_cmd(h);
        if(r)
        {
            printf("ERROR: start_cmd(%d)\n", r);
            return 1;
        }
        printf("connect preloader\n");
    }

    // gen fastboot da from lk
    load_binary(lk_path, &lk, &len_lk);
    strcpy(lk_sig_path, lk_path);
    strcat(lk_sig_path, ".sign");
    load_binary(lk_sig_path, &sig_lk, &len_sig_lk);
    r = send_da(h, chip->lk_wrapper_addr, lk, len_lk, sig_lk, len_sig_lk);
    if(lk) free(lk);
    if(sig_lk) free(sig_lk);
    if(r)
    {
        printf("ERROR: send_da(%d)\n", r);
        return 1;
    }
    printf("send %s\n", lk_path);

    r = jump_da(h, chip->lk_wrapper_addr);
    if(r)
    {
        printf("ERROR: jump_da(%d)\n", r);
        return 1;
    }
    return 0;
}

