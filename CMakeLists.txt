cmake_minimum_required(VERSION 3.15)
project(libfutils VERSION 1.0)

set(DEFS "_FILE_OFFSET_BITS=64")

set(LIB_SOURCES
    src/dynmbox.c
    src/hash.c
    src/mbox.c
    src/systimetools.c
    src/timetools.c
    src/random.c
    src/varint.c
)

add_library(futils ${LIB_SOURCES})
add_library(futils_compiler_flags INTERFACE)
target_compile_features(futils_compiler_flags INTERFACE cxx_std_11)

option(BUILD_SHARED_LIBS "Build using shared libraries" ON)

target_include_directories(futils PUBLIC
                            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                            $<INSTALL_INTERFACE:include>
)

target_compile_definitions(futils PRIVATE ${DEFS})

if(WIN32)
  target_link_libraries(futils ws2_32)
endif()

target_link_libraries(futils 
                        ulog 
                        futils_compiler_flags
)