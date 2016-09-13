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

#include "tty_usb.h"
#include <time.h>
#include <stdio.h>

static const int max_ans_delay = 10;
static uint8_t cmd[] = {0xA0, 0x0A, 0x50, 0x05};
int start_cmd(tty_usb_handle *h)
{
    int i;
    uint8_t r;

    // This should switch to the mode where we can send START_TOKEN
    tty_usb_w8(h, 0xA0);

    {
        struct timespec t={0, 100000000}; // 0.1 second
        nanosleep(&t, NULL);
    }

    tty_usb_flush(h);

    // Now start sending the start token
    for(i = 0; i < sizeof(cmd); i++)
    {
        tty_usb_w8(h, cmd[i]);
    }

    for(i = 0; i < sizeof(cmd); i++)
    {
        r = tty_usb_r8(h);
        if (r != cmd[i])
        {
            fprintf(stderr, "Wrong answer to start token: %02X (should be %02X)\n",
                    r, cmd[i]);
            return -1;
        }
    }

    fprintf(stderr, "Successfuly sent the start token\n");

    {
        struct timespec t={0, 100000000}; // 0.01 second
        nanosleep(&t, NULL);
    }

    tty_usb_flush(h);

    return 0;
}
