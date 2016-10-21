#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

uint16_t method1(int f)
{
  struct stat buf;
  fstat(f, &buf);
  off_t size = buf.st_size;

  uint16_t chksum16 = 0;
  uint16_t data;

  for (off_t i = 0; i < size; i+=2) {
    read(f, &data, 0x02);
    chksum16 ^= data;
  }

  return chksum16;
}

uint16_t method2(int f)
{
  struct stat buf;
  fstat(f, &buf);
  off_t size = buf.st_size;

  uint16_t chksum16 = 0;
  uint8_t data = 0;

  for (off_t i = 0; i < size; ++i) {
    read(f, &data, 0x01);
    chksum16 += data;
  }

  return chksum16;
}

int main(int argc, char *argv[])
{
  if (argc < 2) {
    printf("[Usage] %s filename\n", argv[0]);
    return 1;
  }

  for (unsigned i = 1; i < argc; ++i) {
    printf("File: %s\n", argv[i]);

    int f = open(argv[i], O_RDONLY);
    if (f == -1) {
      printf("Can't open.\n");
      continue;
    }

    uint16_t ret1 = method1(f);
    printf("method1: %04x\n", ret1);

    lseek(f, 0, SEEK_SET);

    uint16_t ret2 = method2(f);
    printf("method2: %04x\n", ret2);

    close(f);
  }

  return 0;
}

