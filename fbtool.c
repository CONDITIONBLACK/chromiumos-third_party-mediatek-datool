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
#include <da_stage2.h>
#include <tty_usb.h>

static void download_part(tty_usb_handle *h, const struct mtk_scatter_header* scatter_h, const struct ptb* ptb)
{
    printf("Configuring %s...\t\t\t", scatter_h->name);
    struct ptb_partition part;
    int8_t part_index = -1;
    {
        for (unsigned i = 0; i < ptb->number; ++i) {
            if (!strncmp(ptb->part[i].name, scatter_h->name, 16)) {
                part_index = i;
                break;
            }
        }

        if (part_index < 0)
            printf("Cannot found %s in PTB.\n", scatter_h->name);

        part = ptb->part[part_index];
    }

    struct mtk_bfbf bfbf = mtk_parse_bfbf(scatter_h->filename);
    int ret = set_part_name(h, scatter_h->name, scatter_h->physical_start_addr, bfbf.size);
    if (ret != 0) {
        printf("[KO]\n");
        return;
    }

    ret = usb_download(h, scatter_h->filename, scatter_h->name, part.part, scatter_h->physical_start_addr, &bfbf);
    if (ret != 0) {
        printf("[KO]\n");
        return;
    }
    printf("[OK]\n");
}

enum INDEX_S {
    INDEX_S_BEGIN = 1 << 0,
    INDEX_S_END   = 1 << 1,
};

static bool to_update_filter(const struct mtk_scatter_header *scatter_h, void *data)
{
    const char* string = data;
    return scatter_h->is_download && !strncmp(scatter_h->operation_type, string, strlen(string));
}

static int stage2_download(tty_usb_handle *h, const struct mtk_scatter_file* scatter_f)
{
    // TODO: check signatures
    // get_rsa(h);

    struct ptb old_ptb = get_partition(h);
    if (old_ptb.number == 0) {
        printf("Cannot get partition table.\nStop downloading.\n");
        return -1;
    }

    struct mtk_scatter_file bootloaders = mtk_filter_scatter_file(scatter_f, &to_update_filter, "BOOTLOADERS");
    if (bootloaders.number != 0)
        printf("Sending bootloaders...");
    for (unsigned i = 0; i < bootloaders.number; ++i)
        send_bootloader(h, &bootloaders.scatters[i]);
    if (bootloaders.number != 0)
        printf("\t\t\t[OK]\n");

    free(bootloaders.scatters);

    struct mtk_scatter_file to_update = mtk_filter_scatter_file(scatter_f, &to_update_filter, "UPDATE");
    for (unsigned i = 0; i < to_update.number; ++i)
        download_part(h, &to_update.scatters[i], &old_ptb);

    {
        struct ptb ptb = get_partition(h);
        if (ptb.number == 0)
            set_partition(h, &old_ptb);
    }

    free(old_ptb.part);
    old_ptb.part = NULL;

    printf("%i file to send\n", to_update.number);
    for (unsigned i = 0; i < to_update.number; ++i) {
        uint8_t index_s = 0;
        if (i == 0)                   index_s += INDEX_S_BEGIN;
        if (i == to_update.number -1) index_s += INDEX_S_END;
        if (send_file(h, &to_update.scatters[i], index_s) != 0)
            return 1;
        free(to_update.scatters);
    }

    return 0;
}

static void stage2_format(tty_usb_handle *h, const struct mtk_disks *disks)
{
    for (unsigned i = 0; i < disks->number; ++i) {
        if (disks->disks[i].size == 0)
            continue;
        set_current_partition(h, disks->disks[i].index);
        read_BMT(h);
        format(h, 0x0, disks->disks[i].size);
        read_BMT(h);
    }

    {
        tty_usb_w8(h, 0xE0);
        tty_usb_w32(h, 0x00);
        tty_usb_r8(h); // 0x5A
    }

    // TODO: Use it to tests signature of bootloader
    // get_rsa(h);

    {
        tty_usb_w8(h, 0xBF);
        tty_usb_r8(h); // 0x5A
        tty_usb_r32(h); // 0x02
    }

    is_BMT_remark(h);
}

int main(int argc, char *argv[])
{
    tty_usb_handle *h = NULL;
    int r, is_brom = 0;
    char *opt_da_path=NULL, *opt_scatter_path=NULL;
    uint16_t chip_code;
    int opt_download = 0;
    int opt_format = 0;

    int c;
    while ((c = getopt(argc, argv, "hdf")) != -1) {
        switch(c)
        {
            case 'h':
                printf("Usage: %s [-h] [-a auth] da scatter\n", argv[0]);
                return 0;
            case 'd':
                opt_download = 1;
                break;
            case 'f':
                opt_format = 1;
                opt_download = 1;
                break;
        }
    }
    if (argv[optind] == NULL || argv[optind+1] == NULL) {
        printf("Usage: %s [-h] [-a auth] da scatter\n", argv[0]);
        return 1;
    }
    opt_da_path = argv[optind];
    opt_scatter_path = argv[optind+1];

    h = tty_usb_open_auto();
    is_brom = tty_usb_is_target_brom();
    if(is_brom) {
        printf("brom is not implemented yet.\n");
        return 1;
    } else printf("connect preloader\n");

    r = start_cmd(h);
    if(r)
    {
        printf("ERROR: start_cmd(%d)\n", r);
        return 1;
    }

    r = get_hw_code(h, &chip_code);
    if(r)
    {
        printf("ERROR: get_hw_code(%d)\n", r);
        return 1;
    }
    printf("chip:%x\n", chip_code);

    if (download_download_agent(h, opt_da_path, chip_code) != 0) {
        printf("Can't send download agent. Abort.\n");
        return 1;
    }

    // Here we have jump in DA
    struct mtk_scatter_file scatter_f = mtk_parse_scatter(opt_scatter_path);
    mtk_print_scatter(&scatter_f);

    // PING/PONG
    if (tty_usb_r8(h) == 0x5A)
        tty_usb_w8(h, 0x5A);

    struct mtk_disks disks = get_disks_info(h);

    // Get USB Speed Status
    printf("USB Speed Status: %x\n", get_usb_speed_status(h));

    {
        tty_usb_w8(h, 0xE0);
        tty_usb_w32(h, 0x00);
        tty_usb_r8(h); // 0x5A
    }

    if (opt_format) {
        // TODO: If format has been choosen all partition need to be rewrite.
        //       Compare scatter_file content with get_partition result
        struct mtk_scatter_file bootloaders = mtk_filter_scatter_file(&scatter_f, &to_update_filter, "BOOTLOADERS");
        if (bootloaders.number == 0) {
            printf("Ask for formatting but no bootloader find in scatter file (%s).\n"
                    "You will not be able to do something after that.\n"
                    "Leaving.\n", opt_scatter_path);
            return 1;
        }
        free(bootloaders.scatters);

        tty_usb_close(h);
        stage2_format(h, &disks);
    }

    if (opt_download) {
        if (stage2_download(h, &scatter_f) != 0)
            printf("Download of files have been aborted due to error.\n");
    }

    free(disks.disks);
    free(scatter_f.scatters);

    check_write(h);

    tty_usb_w8(h, 0xBB);
    tty_usb_r8(h); // 0x5A
    tty_usb_r8(h); // 0x5A ?!?

    exit_DA(h, 0x00);

    tty_usb_close(h);

    return 0;
}

