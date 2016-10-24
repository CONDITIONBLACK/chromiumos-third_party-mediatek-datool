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

#include <tty_usb.h>
#include <stdio.h>
#include <stdint.h>
#define E_ERROR 0x1000
int send_da(tty_usb_handle *h, uint32_t addr, void* da, size_t len_da, void* sig, size_t len_sig)
{
    uint16_t status, checksum=0;
    uint16_t *p;
    int i;

    if(tty_usb_w8_echo(h, 0xD7) != 0) return -1;
    if(tty_usb_w32_echo(h, addr) != 0) return -2;
    if(tty_usb_w32_echo(h, (len_da+len_sig)) != 0) return -3;
    if(tty_usb_w32_echo(h, 0x100) != 0) return -4; //size of package

    status = tty_usb_r16(h);
    if(E_ERROR <= status) return status;

    tty_usb_write(h, da, len_da);
    tty_usb_write(h, sig, len_sig);

    p = (uint16_t*)da;
    for(i=0; i<len_da; i+=2) checksum ^= *p++;
    p = (uint16_t*)sig;
    for(i=0; i<len_sig; i+=2) checksum ^= *p++;

    status = tty_usb_r16(h);
    if(status != checksum) return -5;

    status = tty_usb_r16(h);
    if(E_ERROR <= status) return status;

    return 0;
}

int send_da_part_2(tty_usb_handle *h, uint32_t base_addr, void *da, uint32_t len_da, void *sig, size_t len_sig)
{
    tty_usb_w32(h, base_addr);
    tty_usb_w32(h, len_da+len_sig);
    tty_usb_w32(h, 0x1000); // Size of package

    if (tty_usb_r8(h) != 0x5A) {
        printf("We were waiting for 0x5A\n");
    }

    unsigned cnt = 0;
    uint32_t current = 0;
    printf("Upload: ");
    for (; cnt < len_da/4096; cnt++) {
        tty_usb_write(h, da+cnt*4096, 4096);
        if (tty_usb_r8(h) != 0x5A)
            printf("ACK failed during download!\n");
        uint32_t percent = (cnt+1)*4096*100/len_da;
        while(current < percent) {
            printf("=");
            current++;
        }
    }
    printf("\n");
    tty_usb_write(h, da+cnt*4096, len_da-4096*cnt);
    tty_usb_write(h, sig, len_sig);
    if (tty_usb_r8(h) != 0x5A)
        printf("ACK failed!\n");
}
