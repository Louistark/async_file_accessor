cmake_minimum_required (VERSION 3.0)

set (CMAKE_EXPORT_COMPILE_COMMANDS on)
PROJECT (async_file_accessor C)

################################### SET_ENV ###################################

include (env.cmake)
set (LIB_ASYNC_IO async_io)
set (TEST_ELF async_file_accessor)
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUT_DIR})
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUT_DIR})
set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${OUT_DIR})

############################## INCLUDE_DIRECTORY ##############################

include_directories (${INC_DIR})
include_directories (${SRC_DIR}/aio_file_accessor/)
include_directories (${SRC_DIR}/mmap_file_accessor/)

############################### COMPILE_OPTIONS ###############################

set (COMPILE_CFLAGS "-std=gnu99")
set (CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   ${COMPILE_CFLAGS}")
# set (COMPILE_CPPFLAGS "-std=c++11")
# set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMPILE_CPPFLAGS}")
add_compile_options (${COMPILE_FLAGS})

################################### MACROS ####################################

add_definitions (-DDATA_SET_DIR="$ENV{DATA_SET}/raw_set")
add_definitions (-DOUTPUT_DIR="$ENV{OUT_DIR}/new_files")

################################ LIB_ASYNC_IO #################################

add_library (${LIB_ASYNC_IO} STATIC
    ${SRC_DIR}/async_file_accessor.c
    ${SRC_DIR}/aio_file_accessor/aio_file_accessor.c
    ${SRC_DIR}/mmap_file_accessor/mmap_file_accessor.c
)

target_link_libraries (${LIB_ASYNC_IO} -lrt)

################################## TEST_ELF ###################################

add_executable ( ${TEST_ELF}
    ${ROOT_DIR}/test_async_accessor.c
)

target_link_libraries (${TEST_ELF} ${LIB_ASYNC_IO} -lrt)

################################### INSTALL ###################################

install (TARGETS ${LIB_ASYNC_IO} DESTINATION ${LIB_DIR})