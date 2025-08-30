get_target_property(QT6_BIN_DIR Qt6::Core LOCATION)
get_filename_component(QT6_BIN_DIR "${QT6_BIN_DIR}" DIRECTORY)
if (NOT WIN32 OR NOT EXISTS "${QT6_BIN_DIR}/windeployqt.exe")
    return()
endif()

# install Qt Dlls into the binary dir on windows platforms
if (MSVC)
    add_custom_command(TARGET ${PROJECT_NAME}
        POST_BUILD
        # COMMAND "${QT6_BIN_DIR}/qtenv2.bat"
        COMMAND "${QT6_BIN_DIR}/windeployqt" --no-translations --compiler-runtime "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIGURATION>/${PROJECT_NAME}.exe"
        WORKING_DIRECTORY "${QT6_BIN_DIR}"
        COMMENT "Copy Qt6 dlls for ${PROJECT_NAME}"
    )
else()
    if ("${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo")
        set(ADD_OPT "--release")
    endif()
    add_custom_command(TARGET ${PROJECT_NAME}
        POST_BUILD
        # COMMAND "${QT6_BIN_DIR}/qtenv2.bat"
        COMMAND "${QT6_BIN_DIR}/windeployqt" --no-translations --compiler-runtime ${ADD_OPT} "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.exe"
        WORKING_DIRECTORY "${QT6_BIN_DIR}"
        COMMENT "Copy Qt6 dlls for ${PROJECT_NAME}"
    )
endif()
