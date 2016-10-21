#ifndef LIB_BFBF_LIB_H
#define LIB_BFBF_LIB_H

#include <stdbool.h>
#include <stdint.h>

// BFBF file
struct mtk_bfbf {
  uint32_t sign_addr;
  uint32_t sign_size;
  uint32_t wtf1_addr;
  uint32_t wtf1_size;
  uint32_t wtf2_addr;
  uint32_t wtf2_size;
  uint32_t addr;
  uint32_t size;
};

struct mtk_bfbf mtk_parse_bfbf(const char* filename);

// scatter file
struct mtk_scatter_header {
  char name[16];
  uint32_t is_download;
  uint32_t d_type;
  uint32_t is_reserved;
  uint32_t boundary_check;
  uint64_t partition_size;
  uint64_t linear_start_addr;
  uint64_t physical_start_addr;
  uint32_t reserve;
  char partition_index[16];
  char region[16];
  char storage[16];
  char type[16];
  char operation_type[16];
  char filename[100];
};

struct mtk_scatter_file {
  int number;
  struct mtk_scatter_header *scatters;
};

struct mtk_scatter_file mtk_parse_scatter(const char *filename);
void mtk_scatter_file_free(struct mtk_scatter_file *scatter);
void mtk_print_scatter(const struct mtk_scatter_file* scatter_f);

// MTK DOWNLOAD AGENT file
struct mtk_da_header {
  char magic[32];
  char format[64];
  uint32_t check1; // Should be 4
  uint32_t check2; // Should be 0x22668899
  uint32_t number;
};

struct mtk_da_code_info {
  uint32_t addr;
  uint32_t total_size;
  uint32_t dest;
  uint32_t code_size;
  uint32_t sign_len;
};

struct mtk_da_file_header {
  char magic[2];
  uint16_t hw_code;
  uint32_t hw_version;
  uint32_t fw_version;
  uint32_t extra_version;
  uint32_t foo1;
  struct mtk_da_code_info header;
  struct mtk_da_code_info pld;
  struct mtk_da_code_info da;
  uint32_t zero4[35];
};

struct mtk_da {
  struct mtk_da_header header;
  struct mtk_da_file_header *files_headers;
};

struct mtk_da mtk_da_parse(const char *filename);
void mtk_da_free(struct mtk_da *da);
void mtk_da_print(const struct mtk_da *da);
struct mtk_da_file_header mtk_find_download_agent(const struct mtk_da* da, uint16_t chip_code);

struct mtk_scatter_file mtk_filter_scatter_file(const struct mtk_scatter_file *scatter_f, bool (*filter)(const struct mtk_scatter_header*, void*), void *data);
#endif
