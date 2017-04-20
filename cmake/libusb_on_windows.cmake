if (NOT WIN32)
    return()
endif()

#message("Download libusb1.0")
#if (NOT EXISTS "${CMAKE_BINARY_DIR}/libusb.7z")
#    file(DOWNLOAD "http://downloads.sourceforge.net/project/libusb/libusb-1.0/libusb-1.0.20/libusb-1.0.20.7z?r=http%3A%2F%2Flibusb.info%2F&ts=1457005849&use_mirror=vorboss"
#    "${CMAKE_BINARY_DIR}/libusb.7z" )
#endif()

set(filename "${CMAKE_CURRENT_LIST_DIR}/libusb-1.0.21-win.7z")
set(LIBUSB_DIR "${CMAKE_BINARY_DIR}/libusb-1.0.21-win")

execute_process(
COMMAND ${CMAKE_COMMAND} -E tar xzf "${filename}"
WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
)

set(ARCH "")
if (CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(ARCH "32")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(ARCH "64")
else()
    message(FATAL_ERROR "Target architecture not known")
endif()

target_link_libraries(${PROJECT_NAME} "${LIBUSB_DIR}/${ARCH}/libusb-1.0.lib")
target_include_directories(${PROJECT_NAME} PRIVATE "${LIBUSB_DIR}" "${LIBUSB_DIR}/libusb-1.0")

add_custom_command(TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${LIBUSB_DIR}/${ARCH}/libusb-1.0.dll" $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMENT "Copy libusb-1 dlls for ${PROJECT_NAME}"
)

