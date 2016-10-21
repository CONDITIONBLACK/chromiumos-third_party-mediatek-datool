#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

#include "inc/mtkparse.h"

struct mtk_bfbf mtk_parse_bfbf(const char* filename)
{
  const uint32_t SIGN_ADDR = 0x4000, SIGN_SIZE = 0x40;
  int file = open(filename, O_RDONLY);
  if (file == -1)
    printf("Cannot open file %s.\n", filename);

  struct mtk_bfbf bfbf;

  bfbf.sign_addr = SIGN_ADDR;
  bfbf.sign_size = SIGN_SIZE;

  bfbf.addr = 0x4040;
  {
    lseek(file, 0x4038, SEEK_SET);
    uint32_t length_h; // ignored
    uint32_t length_b;
    read(file, &length_h, sizeof(uint32_t));
    read(file, &length_b, sizeof(uint32_t));
    bfbf.size = length_b;
  }

  bfbf.wtf1_addr = bfbf.addr + bfbf.size;
  bfbf.wtf1_size = 0x94;
  bfbf.wtf2_addr = bfbf.wtf1_addr + bfbf.wtf1_size;
  bfbf.wtf2_size = 0x58;

  close(file);

  return bfbf;
}

// TODO: parse YAML file
struct mtk_scatter_file mtk_parse_scatter(const char *filename)
{
  int file = open(filename, O_RDONLY);
  if (file == -1)
    printf("Cannot open file %s.\n", filename);

  struct stat buf;
  fstat(file, &buf);
  off_t size = buf.st_size;

  struct mtk_scatter_file scatter_f;
  scatter_f.number = size / sizeof(struct mtk_scatter_header);
  scatter_f.scatters = calloc(scatter_f.number, sizeof(struct mtk_scatter_header));

  for (unsigned i = 0; i < scatter_f.number; ++i) {
    read(file, &(scatter_f.scatters[i]), sizeof(struct mtk_scatter_header));
  }
  close(file);

  return scatter_f;
}

void mtk_scatter_file_free(struct mtk_scatter_file *scatter)
{
  free(scatter->scatters);
  scatter->scatters = NULL;
}

struct mtk_da mtk_da_parse(const char *filename)
{
  int file = open(filename, O_RDONLY);
  struct mtk_da_header da_header;
  read(file, &da_header, sizeof(struct mtk_da_header));

  struct mtk_da da;
  da.header = da_header;
  da.files_headers = calloc(da_header.number, sizeof(struct mtk_da_file_header));

  for (unsigned i = 0; i < da_header.number; ++i) {
    struct mtk_da_file_header file_header;
    read(file, &file_header, sizeof(struct mtk_da_file_header));
    da.files_headers[i] = file_header;
  }

  close(file);

  return da;
}

void mtk_da_free(struct mtk_da *da)
{
  free(da->files_headers);
  da->files_headers = NULL;
}

