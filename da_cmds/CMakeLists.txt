set(DA_CMDS_SRC
  start_cmd.c
  get_hw_code.c
  get_target_config.c
  send_auth.c
  send_da.c
  jump_da.c)

add_library(dacmds ${DA_CMDS_SRC})
target_include_directories(dacmds PUBLIC inc)
target_link_libraries(dacmds PUBLIC utils)
