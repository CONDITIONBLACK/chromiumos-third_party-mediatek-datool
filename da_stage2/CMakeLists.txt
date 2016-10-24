set(DA_STAGE2 lib.c)

add_library(dastage2 ${DA_STAGE2})
target_include_directories(dastage2 PUBLIC inc)
target_link_libraries(dastage2 PUBLIC utils mtkparse)
