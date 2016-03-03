message("Download libusb1.0")
if (NOT EXISTS "${CMAKE_BINARY_DIR}/libusb.7z")
    file(DOWNLOAD "http://downloads.sourceforge.net/project/libusb/libusb-1.0/libusb-1.0.20/libusb-1.0.20.7z?r=http%3A%2F%2Flibusb.info%2F&ts=1457005849&use_mirror=vorboss"
    "${CMAKE_BINARY_DIR}/libusb.7z" )
endif()


file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/libusb")

execute_process(
COMMAND ${CMAKE_COMMAND} -E tar xzf ${CMAKE_BINARY_DIR}/libusb.7z
WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/libusb"
)

if (CMAKE_SIZEOF_VOID_P EQUAL 4)
    target_link_libraries(${PROJECT_NAME} "${CMAKE_BINARY_DIR}/libusb/MinGW32/static/libusb-1.0.a")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
    target_link_libraries(${PROJECT_NAME} "${CMAKE_BINARY_DIR}/libusb/MinGW64/static/libusb-1.0.a")
else()
    message(FATAL_ERROR "Target architecture not known")
endif()

include_directories("${CMAKE_BINARY_DIR}/libusb/include" "${CMAKE_BINARY_DIR}/libusb/include/libusb-1.0")
