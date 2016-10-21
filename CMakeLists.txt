cmake_minimum_required(VERSION 3.6.2)
project(datool)

add_subdirectory(libmtkparse)
add_subdirectory(utils)
add_subdirectory(da_cmds)

set(FBTOOL_SRC fbtool.c)

add_executable(fbtool ${FBTOOL_SRC})
target_include_directories(fbtool PRIVATE inc)
target_link_libraries(fbtool mtkparse dacmds)

add_subdirectory(tool)
