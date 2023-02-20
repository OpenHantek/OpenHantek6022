# set file names for associated inf/cat files
set(INF_DIR "${CMAKE_CURRENT_LIST_DIR}/../utils/windows_drivers")
set(DRIVER_DIR "driver")

# use updated cat/inf files provided by VictorEEV
set(OPENHANTEK_CAT "${INF_DIR}/openhantek/OpenHantek.cat")
set(OPENHANTEK_INF "${INF_DIR}/openhantek/OpenHantek.inf")

# execute commands
add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${DRIVER_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${OPENHANTEK_CAT}" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${DRIVER_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${OPENHANTEK_INF}" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${DRIVER_DIR}"
    COMMENT "Copy winusb inf/cat files for ${PROJECT_NAME}"
)
