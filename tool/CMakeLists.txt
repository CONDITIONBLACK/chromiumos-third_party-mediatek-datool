add_executable(chksum ./chksum.c)
add_executable(datool ./datool.c)
target_link_libraries(datool mtkparse)
