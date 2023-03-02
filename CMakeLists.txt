cmake_minimum_required(VERSION 3.15)
project(futils VERSION 1.0)


# checks if set up rpath exists for install
if(COMMAND set_up_rpath)
    set_up_rpath()
else()
    message("Set up rpath not defined!")
endif()

set(DEFS "_FILE_OFFSET_BITS=64")


list(APPEND LIB_SOURCES
    src/hash.c 
    src/mbox.c 
    src/systimetools.c 
    src/timetools.c 
    src/random.c 
    src/varint.c
)

list(APPEND LIB_DEPENDS
    ulog
)

if(UNIX)
    list(APPEND LIB_SOURCES
    src/inotify.c
    )
    if(ANDROID)
        list(APPEND LIB_SOURCES
        src/string.c
        )
    endif()

else()
    list(APPEND LIB_SOURCES
        src/dynmbox.c
        )

endif()



if(NOT WIN32)
    list(APPEND LIB_SOURCES
	src/fdutils.c 
	src/fs.c 
	src/safew.c 
	src/synctools.c
        )

else()
    list(APPEND LIB_DEPENDS
        ws2_32
        )
endif()

add_library(${PROJECT_NAME} ${LIB_SOURCES})
target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_11)

option(BUILD_SHARED_LIBS "Build using shared libraries" ON)
set_target_properties(futils PROPERTIES
                    POSITION_INDEPENDENT_CODE ${BUILD_SHARED_LIBS}
)

target_include_directories(${PROJECT_NAME} PUBLIC
                            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                            $<INSTALL_INTERFACE:include>
)

target_compile_definitions(${PROJECT_NAME} PRIVATE ${DEFS})

target_link_libraries(${PROJECT_NAME} 
                        PRIVATE
                        ${LIB_DEPENDS}
                        )

install(TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}-targets
    PUBLIC_HEADER DESTINATION include
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)
