#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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

void strip_pl_hdr(void *pl, size_t len_pl, void **strip_pl, size_t *len_strip_pl)
{
    EMMC_HEADER_v1 *ehdr = (EMMC_HEADER_v1 *)pl;
    GFH_FILE_INFO_v1 *gfh = (GFH_FILE_INFO_v1 *)pl;
    *strip_pl = pl;
    *len_strip_pl = len_pl;

    // emmc header
    if((strcmp("EMMC_BOOT", ehdr->m_identifier) == 0) &&
       (ehdr->m_ver == 1))
    {
        BR_Layout_v1 *brlyt;

        if(ehdr->m_dev_rw_unit + sizeof(BR_Layout_v1) > len_pl)
        {
            printf("ERROR: EMMC HDR error. dev_rw_unit=%x, brlyt_size=%zx, len_pl=%zx\n",
                        ehdr->m_dev_rw_unit,
                        sizeof(BR_Layout_v1),
                        len_pl);
            exit(EXIT_FAILURE);
        }

        brlyt = (BR_Layout_v1 *)((char *)pl + ehdr->m_dev_rw_unit);
        if((strcmp("BRLYT", brlyt->m_identifier) != 0) ||
           (brlyt->m_ver != 1))
        {
            printf("ERROR: BRLYT error. m_ver=%x, m_identifier=%s\n",
                        brlyt->m_ver,
                        brlyt->m_identifier);
            exit(EXIT_FAILURE);
        }

        gfh = (GFH_FILE_INFO_v1 *)((char *)pl + brlyt->m_bl_desc.m_bl_begin_dev_addr);
    }

    // gfh
    if(((gfh->m_magic_ver & 0x00FFFFFF) == 0x004D4D4D) &&
       (gfh->m_type == 0) &&
       (strcmp("FILE_INFO", gfh->m_identifier) == 0))
    {

        if(gfh->m_file_len < (gfh->m_jump_offset + gfh->m_sig_len))
        {
            // gfh error
            printf("ERROR: GFH error. len_pl=%zx, file_len=%x, jump_offset=%x, sig_len=%x\n",
                        len_pl,
                        gfh->m_file_len,
                        gfh->m_jump_offset,
                        gfh->m_sig_len);
            exit(EXIT_FAILURE);
        }

        *strip_pl = ((char*)gfh + gfh->m_jump_offset);
        *len_strip_pl = gfh->m_file_len - gfh->m_jump_offset - gfh->m_sig_len;
    }
}
