#include <stdint.h>
#include <tty_usb.h>
#include <mtkparse.h>

struct ptb_partition {
  char name[16];
  char foo1[16];
  char foo2[16];
  char foo3[16];

  uint64_t size;
  uint64_t part;
  uint64_t addr;
  
  char foo4[8];
};

struct ptb {
  uint8_t number;
  struct ptb_partition *part;
};

struct rsa {
  char n[256];
  char e[5];
  char seed[16];
};

struct mtk_disk {
  uint8_t index;
  uint64_t size;
};

struct mtk_disks {
  uint8_t number;
  struct mtk_disk *disks;
};

struct mtk_disks get_disks_info(tty_usb_handle *h);

// 0x51
void send_bootloader(tty_usb_handle* h, const struct mtk_scatter_header *bootloader);

// 0x52
bool is_BMT_remark(tty_usb_handle* h);

// 0x60
void set_current_partition(tty_usb_handle* h, uint8_t part_number);

// 0x61
int send_file(tty_usb_handle* h, const struct mtk_scatter_header *scatter_h, uint8_t index_s);

// 0x72
uint8_t get_usb_speed_status(tty_usb_handle* h);

// 0xA3 
void read_BMT(tty_usb_handle* h);

// 0xA4
void set_partition(tty_usb_handle* h, const struct ptb* ptb); 

// 0xA5
// You need to free ptb.part
struct ptb get_partition(tty_usb_handle* h);
void print_PTB(const struct ptb ptb);

// 0xB4
struct rsa get_rsa(tty_usb_handle* h);

// 0xB9
int usb_download(tty_usb_handle* h, const char *filename, const char *partition_name, uint32_t part_number, uint32_t addr, const struct mtk_bfbf *bfbf);

// 0xBA
void check_write(tty_usb_handle* h);

// 0xBE
int set_part_name(tty_usb_handle* h, const char *name, uint64_t img_offset, uint64_t size);

// 0xD4
void format(tty_usb_handle* h, uint64_t addr, uint64_t size);

// 0xD9
void exit_DA(tty_usb_handle* h, uint32_t return_code);

