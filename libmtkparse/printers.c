#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#include "inc/mtkparse.h"

void mtk_print_scatter(const struct mtk_scatter_file* scatter_f)
{
  for (unsigned i = 0; i < scatter_f->number; ++i) {
    const struct mtk_scatter_header *scatter_h = scatter_f->scatters+i;
    if (!scatter_h->is_download)
      continue;
    printf("%s %s %s\t", scatter_h->name, scatter_h->region, scatter_h->filename);
    int f;
    if (f = open(scatter_h->filename, O_RDONLY) < 0) {
      printf(" [KO]\n");
      close(f);
    } else
      printf(" [OK]\n");
  }
}

void mtk_da_print(const struct mtk_da *da)
{
  printf("Name: %s\n", da->header.magic);
  printf("Format: %s\n", da->header.format);
  printf("Gonna read %d headers.\n", da->header.number);

  for (unsigned i = 0; i < da->header.number; ++i) {
    struct mtk_da_file_header file_header = da->files_headers[i];

    printf("[%04x] HW VERSION  = %08x\n", file_header.hw_code, file_header.hw_version);
    printf("[%04x] FW VERSION  = %08x\n", file_header.hw_code, file_header.fw_version);
    puts("");
    printf("[%04x] HEADER ADDR = %08x\n", file_header.hw_code, file_header.header.addr);
    printf("[%04x] HEADER SIZE = %08x\n", file_header.hw_code, file_header.header.total_size);
    puts("");
    printf("[%04x] PLD ADDR       = %08x\n", file_header.hw_code, file_header.pld.addr);
    if (file_header.pld.sign_len == 0)
      printf("[%04x] PLD CODE SIZE  = %08x\n", file_header.hw_code, file_header.pld.total_size);
    else
      printf("[%04x] PLD TOTAL SIZE = %08x\n", file_header.hw_code, file_header.pld.total_size);
    printf("[%04x] PLD DEST ADDR  = %08x\n", file_header.hw_code, file_header.pld.dest);
    if (file_header.pld.sign_len != 0) {
      printf("[%04x] PLD CODE SIZE  = %08x\n", file_header.hw_code, file_header.pld.code_size);
      printf("[%04x] PLD SIGN LEN   = %08x\n", file_header.hw_code, file_header.pld.sign_len);
    }
    puts("");
    printf("[%04x] DA ADDR       = %08x\n", file_header.hw_code, file_header.da.addr);
    if (file_header.da.sign_len == 0)
      printf("[%04x] DA CODE SIZE  = %08x\n", file_header.hw_code, file_header.da.total_size);
    else
      printf("[%04x] DA TOTAL SIZE = %08x\n", file_header.hw_code, file_header.da.total_size);
    printf("[%04x] DA DEST ADDR  = %08x\n", file_header.hw_code, file_header.da.dest);
    if (file_header.da.sign_len != 0) {
      printf("[%04x] DA CODE SIZE  = %08x\n", file_header.hw_code, file_header.da.code_size);
      printf("[%04x] DA SIGN LEN   = %08x\n", file_header.hw_code, file_header.da.sign_len);
    }
    puts("-----------------------------------------------");
  }
}
