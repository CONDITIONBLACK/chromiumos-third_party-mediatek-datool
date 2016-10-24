#include <stdio.h>
#include "inc/da_cmds.h"

static int read_emmc_helper(tty_usb_handle *h)
{
    uint32_t init_status = tty_usb_r32(h);
    if (init_status) {
        printf("Initialization of EMMC failed: %d\n", init_status);
        return 1;
    }
    else
        printf("Initialization of EMMC success.\n");

    printf("EMMC ID: ");
    for (unsigned i = 0; i < 4; ++i) {
        uint32_t emmc_id = tty_usb_r32(h);
        printf("%x ", emmc_id);
    }
    printf("\n");
    return 0;
}

static int read_nand_helper(tty_usb_handle *h)
{
    uint32_t rcv32 = tty_usb_r32(h);
    if (rcv32 != 0xBC4) {
        printf("WARNING: should be 0xBC4: %d\n", rcv32);
        return 1;
    }

    uint16_t nbr = tty_usb_r16(h);
    bool is_nand = false;
    printf("NAND ID: ");
    for (uint16_t i = 0; i < nbr; ++i) {
        uint16_t nand_id = tty_usb_r16(h);
        if (nand_id)
            is_nand = true;
        printf("%04x ", nand_id);
    }
    printf("\n");
    if (!is_nand)
        printf("There is no NAND.\n");
    return 0;
}

int get_emmc_nand_info(tty_usb_handle *h)
{
    {
        {
            read_nand_helper(h);
            read_emmc_helper(h);
        }

        // We answer ok
        tty_usb_w8(h, 0x5A);

        if (tty_usb_r8(h) != 0x04)
            printf("Waiting for 0x04\n");
        if (tty_usb_r8(h) != 0x02)
            printf("Waiting for 0x02\n");
        uint8_t ret8 = tty_usb_r8(h);
        if (ret8 == 0xff) {
            printf("Not implemented yet.\n");
            return 1;
        }
        if (ret8 != 0x8d) {
            printf("Should be 0xff or 0x8d\n");
            return 1;
        }

        tty_usb_w8(h, 0xff);
        tty_usb_w8(h, 0x01);
        tty_usb_w8(h, 0x00);
        tty_usb_w8(h, 0x08);
        tty_usb_w8(h, 0x00);
        // NAND ACC Control
        uint32_t m_nand_acccon = 0x7007ffff;
        tty_usb_w32(h, m_nand_acccon);
        tty_usb_w8(h, 0x01);
        tty_usb_w32(h, 0x00000000);
        tty_usb_w8(h, 0x02);
        tty_usb_w8(h, 0x01);
        tty_usb_w8(h, 0x02);
        tty_usb_w8(h, 0x00);
        tty_usb_w32(h, 0x00000000);
    }

    uint32_t ret32 = tty_usb_r32(h);
    if (ret32 == 0x00000000) {
        // In fact, here we have no idea, if it will continue or do an infinite loop
        // due to RAM not accessible
        return 0;
    }
    if (ret32 != 0xBC3) {
        printf("ERROR: Wrong return: %08x\n", ret32);
        return 1;
    }

    read_emmc_helper(h);
    read_nand_helper(h);

    {
        tty_usb_w8(h, 0xe8);
        uint32_t value = 0x10;
        tty_usb_w32(h, value);
        if (tty_usb_r8(h) != 0x5A) {
            printf("Error when sending preloader.\n");
            return 1;
        }
        if (value != 0xffffffff) {
            tty_usb_r32(h);
            tty_usb_w8(h, 0x5A);
            uint16_t chksum = 0;
            // TODO: send_file...
            uint16_t rcv_chksum = tty_usb_r16(h);
            if (chksum == rcv_chksum) {
                tty_usb_w8(h, 0xa5);
                printf("Checksum isn't valid.\n");
                return 1;
            }
            tty_usb_w8(h, 0x5A);
        }
        tty_usb_w32(h, 0x01);
        tty_usb_r32(h); // 0x00
        tty_usb_r8(h); // 0x02
        tty_usb_r8(h); // 0x00
        tty_usb_r64(h); // 0x40000000

    }

    return 0;
}

