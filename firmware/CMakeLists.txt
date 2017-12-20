project(FirmwareExtractor C)

find_library( BFD_LIBRARY	NAMES bfd libbdf PATH /usr/lib /usr/lib64 )
find_path(BFD_INCLUDE bfd.h PATHS /usr/include
    /usr/local/include
    /opt/local/include
    /sw/include)


if (NOT BFD_LIBRARY OR NOT BFD_INCLUDE)
    message(STATUS "BFD not found. Please install binutils-devel (fedora) / binutils-dev (ubuntu)")
    return()
endif()

add_executable(${PROJECT_NAME} extractfw.c)
target_link_libraries(${PROJECT_NAME} ${BFD_LIBRARY} )
target_include_directories(${PROJECT_NAME} PRIVATE ${BFD_INCLUDE})
