#include <stdlib.h>

#include "inc/mtkparse.h"

// TODO: check hw_version
struct mtk_da_file_header mtk_find_download_agent(const struct mtk_da *da, uint16_t chip_code)
{
  for (unsigned i = 0; i < da->header.number; ++i) {
    if (da->files_headers[i].hw_code == chip_code)
      return da->files_headers[i];
  }
  struct mtk_da_file_header empty;
  return empty;
}

struct mtk_scatter_file mtk_filter_scatter_file(const struct mtk_scatter_file *scatter_f, bool (*filter)(const struct mtk_scatter_header*, void*), void *data)
{
  struct mtk_scatter_file sf;
  sf.scatters = calloc(scatter_f->number, sizeof(struct mtk_scatter_header));
  int number = 0;

  for (unsigned i = 0; i < scatter_f->number; ++i) {
    if (filter(&scatter_f->scatters[i], data))
      sf.scatters[number++] = scatter_f->scatters[i];
  }

  sf.number = number;
  sf.scatters = realloc(sf.scatters, number * sizeof(struct mtk_scatter_header));

  return sf;
}
