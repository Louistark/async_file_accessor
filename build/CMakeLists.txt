cmake_minimum_required(VERSION 3.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS on)
PROJECT(async_file_accessor C)

set (ARMCB_COMPILE_CFLAGS "-std=gnu99")
# set (ARMCB_COMPILE_CPPFLAGS "-std=c++11")
set (CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   ${ARMCB_COMPILE_CFLAGS}")
# set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ARMCB_COMPILE_CPPFLAGS}")
add_compile_options (${ARMCB_COMPILE_FLAGS})

set(ELF async_file_accessor)

add_executable ( ${ELF}
    ../../test_async_accessor.c
    ../../async_file_accessor.c
    ../../aio_file_accessor.c
)

target_link_libraries (${ELF}  -lrt)