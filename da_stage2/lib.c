#include <endian.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include "da_stage2.h"

static int usb_dl(tty_usb_handle* h, uint8_t op)
{
  tty_usb_w8(h, op);
  if (tty_usb_r8(h) != 0x5A) {
    printf("Cannot do operation %02x.\n", op);
    return -1;
  }
  return 0;
}

int usb_download(tty_usb_handle* h, const char *filename, const char *partition_name, uint32_t part_number, uint32_t addr, const struct mtk_bfbf* bfbf)
{
  tty_usb_w8(h, 0xB9);
  if (tty_usb_r8(h) != 0x5A) {
    printf("Cannot start upload of file");
    return 1;
  }

  int file = open(filename, O_RDONLY);
  if (file == -1) {
    printf("Cannot open file to send it.\n");
    return 1;
  }
  {
    // Send partition name
    usb_dl(h, 0x44);
    tty_usb_w32(h, strlen(partition_name));
    tty_usb_write(h, partition_name, strlen(partition_name));

    // Send partition number
    usb_dl(h, 0x66);
    tty_usb_w32(h, part_number);

    // Send signature
    usb_dl(h, 0x11);
    lseek(file, bfbf->sign_addr, SEEK_SET);
    char signature[bfbf->sign_size];
    read(file, signature, bfbf->sign_size);
    tty_usb_w32(h, bfbf->sign_size);
    tty_usb_write(h, signature, bfbf->sign_size);

    // Send address
    usb_dl(h, 0x55);
    tty_usb_w32(h, addr);

    usb_dl(h, 0x22);
    lseek(file, bfbf->wtf1_addr, SEEK_SET);
    char wtf1[bfbf->wtf1_size];
    read(file, wtf1, bfbf->wtf1_size);
    tty_usb_w32(h, bfbf->wtf1_size);
    tty_usb_write(h, wtf1, bfbf->wtf1_size);

    if (usb_dl(h, 0x77) == -1) {
      printf("We cannot write on EMMC. Maybe you need to modify 'SEC_RO'.\n");
      close(file);
      return 1;
    }
    lseek(file, bfbf->wtf2_addr, SEEK_SET);
    char wtf2[bfbf->wtf2_size];
    read(file, wtf2, bfbf->wtf2_size);
    tty_usb_w32(h, bfbf->wtf2_size);
    tty_usb_write(h, wtf2, bfbf->wtf2_size);

    usb_dl(h, 0x33);
    if (tty_usb_r32(h) != 0x00005555) {
      printf("Error with S-USBDL sequence.\n");
      close(file);
      return 1;
    }
  }
  close(file);
  return 0;
}

struct rsa get_rsa(tty_usb_handle* h)
{
  struct rsa rsa;

  tty_usb_w8(h, 0xB4);
  tty_usb_w32(h, 0x0);

  tty_usb_r32(h);
  tty_usb_read(h, rsa.n, 256);
  tty_usb_read(h, rsa.e, 5);
  tty_usb_read(h, rsa.seed, 16);
  printf("img_auth_rsa_n: %256s\n", rsa.n);
  printf("img_auth_rsa_e: %5s\n", rsa.e);
  printf("crypto seed: %16s\n", rsa.seed);

  return rsa;
}

void set_partition(tty_usb_handle* h, const struct ptb* ptb)
{
  tty_usb_w8(h, 0xA4);
  tty_usb_r8(h); // 0x5A

  tty_usb_w32(h, ptb->number);
  tty_usb_r8(h); // 0x5A

  for (unsigned i = 0; i < ptb->number; ++i)
  {
    struct ptb_partition part = ptb->part[i];
    part.size = htobe64(part.size);
    part.part = htobe64(part.part);
    part.addr = htobe64(part.addr);
    tty_usb_write(h, &part, 0x60);
    if (tty_usb_r8(h) != 0x69)
      printf("Error while setting partition %16s\n", part.name);
  }

  tty_usb_w32(h, 0x0);
  tty_usb_r8(h); // 0x5A
  tty_usb_r8(h); // 0x5A
}

struct ptb get_partition(tty_usb_handle* h)
{
    tty_usb_w8(h, 0xA5);

    struct ptb ptb;
    uint8_t ret = tty_usb_r8(h);
    uint32_t size_of_ptb = tty_usb_r32(h);
    if (ret == 0xA5) { // Can't get PTB because format
      ptb.number = 0;
      ptb.part = NULL;
      return ptb;
    }
    tty_usb_w8(h, 0x5A);
    struct ptb_partition *data = malloc(size_of_ptb);
    size_t len = tty_usb_read(h, data, size_of_ptb);
    if (len == size_of_ptb)
      tty_usb_w8(h, 0x5A);
    else
      printf("PTB receive is not good!\n");
    ptb.number = size_of_ptb/sizeof(struct ptb_partition);
    printf("PTB received %d entries\n", ptb.number);
    ptb.part = calloc(sizeof(struct ptb_partition), ptb.number);
    for (unsigned i = 0; i < ptb.number; i++) {
      ptb.part[i] = data[i];
      ptb.part[i].size = be64toh(ptb.part[i].size);
      ptb.part[i].part = be64toh(ptb.part[i].part);
      ptb.part[i].addr = be64toh(ptb.part[i].addr);
    }
    free(data);

    return ptb;
}

void print_PTB(const struct ptb ptb)
{
  for (unsigned i = 0; i < ptb.number; ++i) {
    printf("%d: %s\t, partition: %2lu, address: 0x%08x, size: 0x%08x\n", i, ptb.part[i].name, ptb.part[i].part, ptb.part[i].addr, ptb.part[i].size);
  }
}

uint8_t get_usb_speed_status(tty_usb_handle* h)
{
  tty_usb_w8(h, 0x72);
  uint8_t ret;
  if (ret = tty_usb_r8(h) != 0x5A)
    printf("ACK should be 0x5A: %x\n", ret);
  return tty_usb_r8(h);
}

static int write_data(tty_usb_handle *h, int file, uint32_t length, uint32_t chksum_each_size)
{
  printf("Gonna write %08x bytes\n", length);
  uint16_t chksum = 0;
  uint16_t complete_chksum = 0;

  uint32_t size_of_packet = 0x4000;
  char *data = malloc(size_of_packet);

  uint32_t number = length / size_of_packet;
  for (unsigned cnt = 0; cnt < number; ++cnt) {
    read(file, data, size_of_packet);
    tty_usb_write(h, data, size_of_packet);
    for (unsigned i = 0; i < size_of_packet; ++i)
      chksum += data[i];
    if (((cnt+1) * size_of_packet) % chksum_each_size == 0) {
      tty_usb_w16(h, chksum);
      complete_chksum += chksum;
      chksum = 0;
      if (tty_usb_r8(h) != 0x69)
        return -1;
      else
        tty_usb_w8(h, 0x5A);
    }
  }

  if (length % size_of_packet) {
    read(file, data, length % size_of_packet);
    tty_usb_write(h, data, length % size_of_packet);
    for (unsigned i = 0; i < length % size_of_packet; ++i)
      chksum += data[i];
    complete_chksum += chksum;
  }

  free(data);

  tty_usb_w16(h, chksum);
  if (tty_usb_r8(h) != 0x69) {
    printf("Bad partial checksum\n");
    return -1;
  }

  tty_usb_w16(h, complete_chksum);
  if (tty_usb_r8(h) != 0x5A) {
    printf("Bad global checksum\n");
    return -1;
  }

  return 0;
}

int send_file(tty_usb_handle* h, const struct mtk_scatter_header *scatter_h, uint8_t index_s)
{
  tty_usb_w8(h, 0x61);

  struct mtk_bfbf bfbf = mtk_parse_bfbf(scatter_h->filename);
  int file = open(scatter_h->filename, O_RDONLY);
  if (file == -1)
    printf("Cannot open file (%s) to upload it to %s.\n", scatter_h->filename, scatter_h->name);

  uint8_t imgIndex = atoi(scatter_h->partition_index+3);
  printf("Gonna send file '%s' for '%s' in %d at 0x%08x\n", scatter_h->filename, scatter_h->name, imgIndex, scatter_h->physical_start_addr);

  tty_usb_w8(h, 0x00);
  // TODO: find how link name -> number
  // Following code write in 2 or 3 is refused.
  if      (!strncmp(scatter_h->region, "EMMC_BOOT_1", 11))
    tty_usb_w8(h, 1);
  else if (!strncmp(scatter_h->region, "EMMC_BOOT_2", 11))
    tty_usb_w8(h, 2);
  else if (!strncmp(scatter_h->region, "EMMC_BOOT_3", 11))
    tty_usb_w8(h, 3);
  else if (!strncmp(scatter_h->region, "EMMC_USER", 9))
    tty_usb_w8(h, 8);
  tty_usb_w64(h, scatter_h->physical_start_addr);
  tty_usb_w64(h, bfbf.size);

  tty_usb_w8(h, imgIndex);
  tty_usb_w8(h, index_s);

  uint32_t chksum_each_size = tty_usb_r32(h);
  uint8_t ret = tty_usb_r8(h);
  if (ret != 0x5A) { // Check of physical start addr (addr << 0x17 == 0)
    printf("Problem during send of %s (0x%02x).\n", scatter_h->name, ret);
    return 1;
  }
  tty_usb_w8(h, 0x5A); // 0x5A: OK, 0x96: abort

  {
    lseek(file, bfbf.addr, SEEK_SET);
    int ret = write_data(h, file, bfbf.size, chksum_each_size);
    if (ret == -1) {
      printf("Bad checksum during upload of %s.\n", scatter_h->filename);
      return 1;
    }
  }

  close(file);
  return 0;
}

void exit_DA(tty_usb_handle* h, uint32_t return_code)
{
  tty_usb_w8(h, 0xd9);
  tty_usb_r8(h); // 0x5A
  tty_usb_w32(h, return_code); // Value is device status
  tty_usb_r8(h); // 0x5A
}

void read_BMT(tty_usb_handle* h)
{
  tty_usb_w8(h, 0xA3);
  if (tty_usb_r8(h) != 0x5A) {
    printf("Error for 0xA3\n");
  }
  uint8_t ret = tty_usb_r8(h);
  tty_usb_w8(h, 0x5A); // Everything allright
}

void set_current_partition(tty_usb_handle* h, uint8_t part_number)
{
  printf("Set current partition to %d\n.", part_number);
  tty_usb_w8(h, 0x60); // Set current partition
  if (tty_usb_r8(h) != 0x5A) {
    printf("Error for 0x60\n");
  }
  tty_usb_w8(h, part_number); // partition number
  tty_usb_r8(h); // 0x5A
}

void format(tty_usb_handle* h, uint64_t addr, uint64_t size)
{
  uint8_t ret;
  tty_usb_w8(h, 0xd4); // Format

  tty_usb_w8(h, 0x02);
  tty_usb_w8(h, 0x00);
  tty_usb_w8(h, 0x00);
  tty_usb_w8(h, 0x01);
  tty_usb_w64(h, addr); // Address
  tty_usb_w64(h, size); // Size
  if ((ret = tty_usb_r8(h)) != 0x5A)
    printf("Error1: 0x%02x instead of 0x5A\n", ret);

  if ((ret = tty_usb_r8(h)) != 0x5A)
    printf("Error2: 0x%02x instead of 0x5A\n", ret);
  tty_usb_r32(h); // 0x00

  if ((ret = tty_usb_r8(h)) != 0x64) // constant
    printf("Error3: 0x%02x instead of 0x64\n");
  tty_usb_w8(h, 0x5A); // OK for constant

  if ((ret = tty_usb_r8(h)) !=  0x5A)
    printf("Error4: 0x%02x instead of 0x5A\n");
  if ((ret = tty_usb_r8(h)) != 0x5A) // Format DONE
    printf("Error5: 0x%02x instead of 0x5A\n");
}

bool is_BMT_remark(tty_usb_handle* h)
{
  tty_usb_w8(h, 0x52);
  uint8_t ret = tty_usb_r8(h);
  if (ret == 0x5A)
    return true; // ACK
  else if (ret == 0xA5)
    return false; // NACK
  return false; // ?!?
}

int set_part_name(tty_usb_handle* h, const char* name, uint64_t img_offset, uint64_t size)
{
  tty_usb_w8(h, 0xBE);

  char _name[16];
  bzero(_name, 16);
  strncpy(_name, name, 16);
  tty_usb_write(h, _name, 16);
  if (tty_usb_r8(h) != 0x5A) {
    printf("Can't tell %s\n", name);
    return 1;
  }

  tty_usb_w64(h, img_offset);
  if (tty_usb_r8(h) != 0x5A) {
    printf("Can't modify %s\n", name);
    return 1;
  }

  // A size on 64 bits;
  tty_usb_w64(h, size);
  if (tty_usb_r8(h) != 0x5A) {
    printf("Can't modify %s\n", name);
    return 1;
  }

  if (tty_usb_r32(h) != 0x43434343) { // "CCCC" ATTR_SEC_IMG_COMPLETE or SEC_CFG_COMPLETE_NUM 
    printf("Config fail for %s\n", name);
    return 1;
  }
  return 0;
}

void send_bootloader(tty_usb_handle* h, const struct mtk_scatter_header *bootloader)
{
  tty_usb_w8(h, 0x51);

  // 160 bits: SHA 1?
  char str[] = "\xC4\x72\xe2\xe7\x7d\x21\x2b\xd4\x63\x31\xe3\x71\x69\xe0\xbe\x84\x47\xa6\xbd\x33\x24\x06\xc5\x80\x06\x56\xf0\x6f\x8e\x8e\x09\x7a";

  const uint32_t SIZE_OF_PACKAGE = 0x2000;

  // Load bootloader
  int f = open(bootloader->filename, O_RDONLY);
  struct stat buf;
  fstat(f, &buf);
  off_t bootloader_size = buf.st_size;

  tty_usb_r8(h); // 0x5A
  tty_usb_w32(h, strlen(str));
  tty_usb_write(h, str, strlen(str));
  tty_usb_r32(h); // 0x00 constant
  tty_usb_w32(h, SIZE_OF_PACKAGE); // Size of package

  tty_usb_w8(h, 0x05); // there is a switch on that
  tty_usb_r8(h); // 0x5A
  tty_usb_r8(h); // 0x5A
  tty_usb_w32(h, 0x1);
  tty_usb_w8(h, 0x5A);
  tty_usb_w8(h, 0x05);
  tty_usb_w16(h, 0x1);
  tty_usb_w32(h, 0x0);
  tty_usb_w32(h, 0x0);
  tty_usb_w32(h, 0x1);
  tty_usb_w32(h, 0x20000);
  tty_usb_w8(h, 0x0);
  tty_usb_w32(h, 0x300);
  tty_usb_w32(h, 0x0);
  tty_usb_w32(h, bootloader_size);
  tty_usb_w8(h, 0x0);
  tty_usb_w8(h, 0x0);
  tty_usb_r8(h); // 0x5A
  tty_usb_r8(h); // 0x5A
  tty_usb_w8(h, 0x5A);

  uint32_t count = bootloader_size / SIZE_OF_PACKAGE;
  uint32_t end = bootloader_size % SIZE_OF_PACKAGE;
  char data[SIZE_OF_PACKAGE];
  for (uint32_t i = 0; i < count; ++i)
  {
    read(f, data, SIZE_OF_PACKAGE);
    tty_usb_write(h, data, SIZE_OF_PACKAGE);

    {
      uint16_t chksum = 0;
      for (uint32_t j = 0; j < SIZE_OF_PACKAGE; j++)
        chksum += data[j];
      tty_usb_w16(h, chksum);
      if (tty_usb_r8(h) != 0x69)
        printf("Error with chksum\n");
    }
  }

  read(f, data, end);
  tty_usb_write(h, data, end);
  uint16_t chksum = 0;
  for (uint32_t j = 0; j < end; j++)
    chksum += data[j];
  tty_usb_w16(h, chksum);
  if (tty_usb_r8(h) != 0x69)
    printf("Error with chksum\n");
  if (tty_usb_r8(h) != 0x5A)
    printf("Error with format\n");
  close(f);
} 

struct mtk_disks get_disks_info(tty_usb_handle *h)
{
  tty_usb_r32(h); // 0xbc7
  tty_usb_r8(h);  // 0x0
  struct mtk_disks disks;
  disks.number = tty_usb_r8(h);
  disks.disks = calloc(disks.number, sizeof(struct mtk_disk));
  tty_usb_r16(h); // 0xffff
  tty_usb_r32(h); // 0x0
  tty_usb_r16(h); // 0x0
  tty_usb_r16(h); // 0x0
  tty_usb_r16(h); // 0x0
  tty_usb_r16(h); // 0x0
  tty_usb_r32(h); // 0x0
  tty_usb_r32(h); // 0x0
  tty_usb_r32(h); // 0xbc8
  tty_usb_r8(h);  // 0x0
  tty_usb_r16(h); // 0xffff
  tty_usb_r32(h); // 0x0
  tty_usb_r32(h); // 0x0
  tty_usb_r16(h); // 0x7
  tty_usb_r16(h); // 0x0
  tty_usb_r16(h); // 0x0
  tty_usb_r16(h); // 0x0
  tty_usb_r16(h); // 0x0
  tty_usb_r16(h); // 0x0
  tty_usb_r16(h); // 0x0
  tty_usb_r16(h); // 0x0
  tty_usb_r16(h); // 0x0
  tty_usb_r16(h); // 0x0
  tty_usb_r16(h); // 0x0
  tty_usb_r8(h);  // 0x0
  tty_usb_r8(h);  // 0x0
  tty_usb_r8(h);  // 0x0
  tty_usb_r32(h); // 0x0

  for (unsigned i = 0; i < disks.number; ++i) {
    disks.disks[i].index = i+1;
    disks.disks[i].size = tty_usb_r64(h);
  }

  tty_usb_r32(h);
  tty_usb_r32(h);
  tty_usb_r32(h);
  tty_usb_r32(h);
  tty_usb_r8(h);
  tty_usb_r8(h);
  tty_usb_r8(h);
  tty_usb_r8(h);
  tty_usb_r8(h);
  tty_usb_r8(h);
  tty_usb_r8(h);
  tty_usb_r8(h);
  tty_usb_r32(h); // 0xc4f
  tty_usb_r32(h); // 0x0
  tty_usb_r32(h); // 0x0
  tty_usb_r32(h); // 0x0
  tty_usb_r32(h); // 0x0
  tty_usb_r32(h); // 0x0
  tty_usb_r32(h); // 0x0
  tty_usb_r32(h); // 0x0
  tty_usb_r32(h); // 0x20000 // SRAM size
  tty_usb_r32(h); // 0x0
  tty_usb_r8(h);  // 0x2
  tty_usb_r8(h);  // 0x0
  tty_usb_r32(h); // 0x0
  tty_usb_r32(h); // 0x40000000 // EXT DRAM size
  for (unsigned i = 0; i < 16; ++i)
    tty_usb_r8(h);
  tty_usb_r32(h);
  tty_usb_r8(h);
  tty_usb_r32(h); // 0xc0b
  tty_usb_r32(h);
  tty_usb_r8(h);

  return disks;
}

void check_write(tty_usb_handle *h)
{
  // Even if upload doesn't work it return 0x5A 0x00000000
  tty_usb_w8(h, 0xba);
  tty_usb_r8(h); // 0x5A
  tty_usb_r32(h); // 0x00000000
}
