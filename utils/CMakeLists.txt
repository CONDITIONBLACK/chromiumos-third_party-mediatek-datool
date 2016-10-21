set(UTILS_SRC 
  file_util.c
  tty_usb_common.c
  tty_usb_linux.c)

add_library(utils ${UTILS_SRC})
target_include_directories(utils PUBLIC inc)
