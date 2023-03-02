cmake_minimum_required(VERSION 3.15)
project(futils VERSION 1.0)


# checks if set up rpath exists for install
if(COMMAND set_up_rpath)
    set_up_rpath()
else()
    message("Set up rpath not defined!")
endif()

set(DEFS "_FILE_OFFSET_BITS=64")

set(LIB_SOURCES
    src/hash.c
    src/mbox.c
    src/systimetools.c
    src/timetools.c
    src/random.c
    src/varint.c
    src/dynmbox.c
)
if(UNIX)
  set(LIB_SOURCES
        ${LIB_SOURCES}
        src/fdutils.c
        src/fs.c
        src/safew.c
        src/synctools.c
        src/inotify.c
        src/string.c
  )
endif()

add_library(${PROJECT_NAME} ${LIB_SOURCES})
target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_11)

option(BUILD_SHARED_LIBS "Build using shared libraries" ON)

target_include_directories(${PROJECT_NAME} PUBLIC
                            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                            $<INSTALL_INTERFACE:include>
)

target_compile_definitions(${PROJECT_NAME} PRIVATE ${DEFS})

if(WIN32)
target_link_libraries(${PROJECT_NAME} 
                        PRIVATE
                        ws2_32
                        ulog 
                        )
else()
target_link_libraries(${PROJECT_NAME} 
                      PRIVATE
                      ulog 
                    )
endif()

install(TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}-targets
    PUBLIC_HEADER DESTINATION include
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)
