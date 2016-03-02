# install Qt Dlls into the binary dir on windows platforms
get_target_property(QT5_BIN_DIR Qt5::Core LOCATION)
get_filename_component(QT5_BIN_DIR "${QT5_BIN_DIR}" DIRECTORY)
if (MSVC AND EXISTS "${QT5_BIN_DIR}/windeployqt.exe")
        add_custom_command(TARGET ${PROJECT_NAME}
            POST_BUILD
            COMMAND "${QT5_BIN_DIR}/qtenv2.bat"
            COMMAND "${QT5_BIN_DIR}/windeployqt" --no-translations "${CMAKE_BINARY_DIR}/$<CONFIGURATION>/${PROJECT_NAME}.exe"
            WORKING_DIRECTORY "${QT5_BIN_DIR}"
            COMMENT "Copy Qt5 dlls for ${PROJECT_NAME}"
        )

    SET(EXE "\${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.exe")
	configure_file(installQt.cmake.in "${CMAKE_BINARY_DIR}/installQt.cmake")
	install(SCRIPT "${CMAKE_BINARY_DIR}/installQt.cmake")
endif()
