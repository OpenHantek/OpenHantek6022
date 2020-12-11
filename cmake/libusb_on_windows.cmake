if (NOT WIN32)
    return()
endif()

#message("Download libusb1.0")
#if (NOT EXISTS "${CMAKE_BINARY_DIR}/libusb.7z")
#    file(DOWNLOAD "http://downloads.sourceforge.net/project/libusb/libusb-1.0/libusb-1.0.20/libusb-1.0.20.7z?r=http%3A%2F%2Flibusb.info%2F&ts=1457005849&use_mirror=vorboss"
#    "${CMAKE_BINARY_DIR}/libusb.7z" )
#endif()

# set file names for lib and associated inf/cat files
set(LIBUSB_7Z "${CMAKE_CURRENT_LIST_DIR}/libusb-1.0.21-win.7z")
set(HANTEK_6022B_CAT "${CMAKE_CURRENT_LIST_DIR}/Hantek_6022B.cat")
set(HANTEK_6022B_INF "${CMAKE_CURRENT_LIST_DIR}/Hantek_6022B.inf")
set(LIBUSB_DIR "${CMAKE_BINARY_DIR}/libusb-1.0.21-win")

execute_process(
COMMAND ${CMAKE_COMMAND} -E tar xzf "${LIBUSB_7Z}"
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

# execute commands
add_custom_command(TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${LIBUSB_DIR}/${ARCH}/libusb-1.0.dll" $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${HANTEK_6022B_CAT}" $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${HANTEK_6022B_INF}" $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMENT "Copy libusb-1 dlls and inf/cat files for ${PROJECT_NAME}"
)

