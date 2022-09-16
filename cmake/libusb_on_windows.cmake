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
# use updated cat/inf files provided by VictorEEV
set(HANTEK_6022B_CAT "${CMAKE_CURRENT_LIST_DIR}/OpenHantek_driver/OpenHantek.cat")
set(HANTEK_6022B_INF "${CMAKE_CURRENT_LIST_DIR}/OpenHantek_driver/OpenHantek.inf")
# use "new" cat/inf files provided by fgrieu (PR #251)
set(INF_DIR "${CMAKE_CURRENT_LIST_DIR}/../utils/signed-windows-inf-files")
set(HANTEK_6022BE_LOADER_CAT "${INF_DIR}/Hantek_6022BE_loader.cat")
set(HANTEK_6022BE_LOADER_INF "${INF_DIR}/Hantek_6022BE_loader.inf")
set(HANTEK_6022BE_OPENHT_CAT "${INF_DIR}/Hantek_6022BE_openht.cat")
set(HANTEK_6022BE_OPENHT_INF "${INF_DIR}/Hantek_6022BE_openht.inf")
set(HANTEK_6022BE_SIGROK_CAT "${INF_DIR}/Hantek_6022BE_sigrok.cat")
set(HANTEK_6022BE_SIGROK_INF "${INF_DIR}/Hantek_6022BE_sigrok.inf")
set(HANTEK_6022BL_LOADER_CAT "${INF_DIR}/Hantek_6022BL_loader.cat")
set(HANTEK_6022BL_LOADER_INF "${INF_DIR}/Hantek_6022BL_loader.inf")
set(HANTEK_6022BL_OPENHT_CAT "${INF_DIR}/Hantek_6022BL_openht.cat")
set(HANTEK_6022BL_OPENHT_INF "${INF_DIR}/Hantek_6022BL_openht.inf")
set(HANTEK_6022BL_SIGROK_CAT "${INF_DIR}/Hantek_6022BL_sigrok.cat")
set(HANTEK_6022BL_SIGROK_INF "${INF_DIR}/Hantek_6022BL_sigrok.inf")
set(README_INSTALL_TXT "${INF_DIR}/README_INSTALL.txt")
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
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${HANTEK_6022BE_LOADER_CAT}" $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${HANTEK_6022BE_LOADER_INF}" $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${HANTEK_6022BE_OPENHT_CAT}" $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${HANTEK_6022BE_OPENHT_INF}" $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${HANTEK_6022BE_SIGROK_CAT}" $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${HANTEK_6022BE_SIGROK_INF}" $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${HANTEK_6022BL_LOADER_CAT}" $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${HANTEK_6022BL_LOADER_INF}" $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${HANTEK_6022BL_OPENHT_CAT}" $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${HANTEK_6022BL_OPENHT_INF}" $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${HANTEK_6022BL_SIGROK_CAT}" $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${HANTEK_6022BL_SIGROK_INF}" $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${README_INSTALL_TXT}" $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMENT "Copy libusb-1 dlls and inf/cat files for ${PROJECT_NAME}"
)

