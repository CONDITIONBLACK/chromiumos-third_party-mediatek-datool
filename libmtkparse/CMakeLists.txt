set(MTKPARSE_SRC
  helpers.c
  inc/mtkparse.h
  parsers.c
  printers.c)
add_library(mtkparse STATIC ${MTKPARSE_SRC})
target_include_directories(mtkparse PUBLIC inc)
