/*
 * Copyright (c) 2014 MediaTek Inc.
 * Author: Tristan Shieh <tristan.shieh@mediatek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <mtkparse.h>
#include <da_cmds.h>
#include <tty_usb.h>

int main(int argc, char *argv[])
{
    tty_usb_handle *h = NULL;
    int r, is_brom = 0;
    char *auth_path=NULL, *pl_path=NULL, *lk_path=NULL;
    uint16_t chip_code;

    int c;
    while ((c = getopt(argc, argv, "ha:")) != -1) {
        switch(c)
        {
            case 'h':
                printf("Usage: %s [-h] [-a auth] preloader lk\n", argv[0]);
                return 0;
            case 'a':
                auth_path = optarg;
                break;
        }
    }
    if (argv[optind] == NULL || argv[optind+1] == NULL) {
        printf("Usage: %s [-h] [-a auth] preloader lk\n", argv[0]);
        return 1;
    }
    pl_path = argv[optind];
    lk_path = argv[optind+1];

    h = tty_usb_open_auto();
    is_brom = tty_usb_is_target_brom();

    r = start_cmd(h);
    if(r)
    {
        printf("ERROR: start_cmd(%d)\n", r);
        return 1;
    }
    if(is_brom) printf("connect brom\n");
    else printf("connect preloader\n");

    r = get_hw_code(h, &chip_code);
    if(r)
    {
        printf("ERROR: get_hw_code(%d)\n", r);
        return 1;
    }
    printf("chip:%x\n", chip_code);

    if (download_download_agent(h, is_brom, auth_path, pl_path, lk_path, chip_code) != 0) {
        printf("Can't send download agent. Abort.\n");
        return 1;
    }

    tty_usb_close(h);

    return 0;
}

