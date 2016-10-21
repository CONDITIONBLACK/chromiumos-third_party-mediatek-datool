cmake_minimum_required(VERSION 3.6.2)
project(datool)

add_subdirectory(libmtkparse)

set(FBTOOL_SRC
  fbtool.c
  cmds/start_cmd.c
  cmds/get_hw_code.c
  cmds/get_target_config.c
  cmds/send_auth.c
  cmds/send_da.c
  cmds/jump_da.c
  utils/file_util.c
  utils/tty_usb_common.c
  utils/tty_usb_linux.c)

add_executable(fbtool ${FBTOOL_SRC})
target_include_directories(fbtool PRIVATE inc)

add_subdirectory(tool)
