#include <stdlib.h>
#include <stdint.h>
#include <endian.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <mtkparse.h>

struct DA_HEADER {
  char magic[32];
  char format[64];
  uint32_t foo1;
  uint32_t foo2;
  uint32_t number;
};

struct CODE_INFO {
  uint32_t addr;
  uint32_t total_size;
  uint32_t dest;
  uint32_t code_size;
  uint32_t sign_len;
};

struct FILE_HEADER {
  char magic[2];
  uint16_t hw_code;
  uint32_t hw_version;
  uint32_t zero1;
  uint32_t foo1;
  uint32_t foo2;
  uint32_t header_addr;
  uint32_t header_size;
  uint32_t foo3;
  uint32_t zero2;
  uint32_t zero3;
  struct CODE_INFO pld;
  struct CODE_INFO da;
  uint32_t zero4[35];
};

int main(int argc, char *argv[])
{
  uint16_t chip_code = 0;
  const char *filename = NULL;

  int c;
  while ((c = getopt(argc, argv, "hc:")) != -1) {
    switch(c) {
    case 'h':
      printf("Usage: %s [-h] [-c chip_code] filename.\n", argv[0]);
      return 0;
    case 'c':
      chip_code = strtol(optarg, NULL, 0);
      break;
    }
  }

  if (argv[optind] == NULL) {
    printf("Usage: %s [-h] [-c chip_code] filename.\n", argv[0]);
    return 1;
  }

  filename = argv[optind];

  struct mtk_da da = mtk_da_parse(filename);
  mtk_da_print(&da);
  if (chip_code != 0) {
    // To implement
  }
  mtk_da_free(&da);
}
