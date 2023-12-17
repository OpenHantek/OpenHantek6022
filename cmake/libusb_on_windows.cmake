message("libusb_on_windows")

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
set(LIBUSB_DIR "${CMAKE_BINARY_DIR}/libusb-1.0.21-win")
set(INF_DIR "${CMAKE_CURRENT_LIST_DIR}/../utils/windows_drivers")
set(DRIVER_DIR "driver")

if (USE_OPENHANTEK_DRIVER)
    # use updated cat/inf files provided by VictorEEV
    set(OPENHANTEK_CAT "${INF_DIR}/openhantek/OpenHantek.cat")
    set(OPENHANTEK_INF "${INF_DIR}/openhantek/OpenHantek.inf")
else()
    # use "new" cat/inf files provided by fgrieu (PR #251)
    set(HANTEK_6022BE_LOADER_CAT "${INF_DIR}/fgrieu/Hantek_6022BE_loader.cat")
    set(HANTEK_6022BE_LOADER_INF "${INF_DIR}/fgrieu/Hantek_6022BE_loader.inf")
    set(HANTEK_6022BE_OPENHT_CAT "${INF_DIR}/fgrieu/Hantek_6022BE_openht.cat")
    set(HANTEK_6022BE_OPENHT_INF "${INF_DIR}/fgrieu/Hantek_6022BE_openht.inf")
    set(HANTEK_6022BE_SIGROK_CAT "${INF_DIR}/fgrieu/Hantek_6022BE_sigrok.cat")
    set(HANTEK_6022BE_SIGROK_INF "${INF_DIR}/fgrieu/Hantek_6022BE_sigrok.inf")
    set(HANTEK_6022BL_LOADER_CAT "${INF_DIR}/fgrieu/Hantek_6022BL_loader.cat")
    set(HANTEK_6022BL_LOADER_INF "${INF_DIR}/fgrieu/Hantek_6022BL_loader.inf")
    set(HANTEK_6022BL_OPENHT_CAT "${INF_DIR}/fgrieu/Hantek_6022BL_openht.cat")
    set(HANTEK_6022BL_OPENHT_INF "${INF_DIR}/fgrieu/Hantek_6022BL_openht.inf")
    set(HANTEK_6022BL_SIGROK_CAT "${INF_DIR}/fgrieu/Hantek_6022BL_sigrok.cat")
    set(HANTEK_6022BL_SIGROK_INF "${INF_DIR}/fgrieu/Hantek_6022BL_sigrok.inf")
    set(README_INSTALL_TXT "${INF_DIR}/fgrieu/README_INSTALL.txt")
endif()

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
if (USE_OPENHANTEK_DRIVER)
    add_custom_command(TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${DRIVER_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${OPENHANTEK_CAT}" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${DRIVER_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${OPENHANTEK_INF}" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${DRIVER_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${LIBUSB_DIR}/${ARCH}/libusb-1.0.dll" "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
        COMMENT "Copy libusb-1 dlls and inf/cat files for ${PROJECT_NAME}"
    )
else()
    add_custom_command(TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${DRIVER_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${HANTEK_6022BE_LOADER_CAT}" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${DRIVER_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${HANTEK_6022BE_LOADER_INF}" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${DRIVER_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${HANTEK_6022BE_OPENHT_CAT}" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${DRIVER_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${HANTEK_6022BE_OPENHT_INF}" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${DRIVER_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${HANTEK_6022BE_SIGROK_CAT}" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${DRIVER_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${HANTEK_6022BE_SIGROK_INF}" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${DRIVER_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${HANTEK_6022BL_LOADER_CAT}" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${DRIVER_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${HANTEK_6022BL_LOADER_INF}" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${DRIVER_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${HANTEK_6022BL_OPENHT_CAT}" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${DRIVER_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${HANTEK_6022BL_OPENHT_INF}" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${DRIVER_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${HANTEK_6022BL_SIGROK_CAT}" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${DRIVER_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${HANTEK_6022BL_SIGROK_INF}" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${DRIVER_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${README_INSTALL_TXT}" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${DRIVER_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${LIBUSB_DIR}/${ARCH}/libusb-1.0.dll" "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
        COMMENT "Copy libusb-1 dlls and inf/cat files for ${PROJECT_NAME}"
    )
endif()
