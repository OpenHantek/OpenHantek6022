set_directory_properties(PROPERTIES CLEAN_NO_CUSTOM 1)

find_package(Qt5LinguistTools QUIET)

if (NOT Qt5LinguistTools_FOUND)
    message(STATUS "Will not build translations! Please install Qt5LinguistTools")
	# Copy empty translations.qrc file to not make the build fail because of this.
	configure_file("${CMAKE_CURRENT_LIST_DIR}/translations.qrc" "${CMAKE_BINARY_DIR}/translations.qrc" @ONLY)
    return()
endif()

qt5_create_translation(TRANSLATION_BIN_FILES "${CMAKE_CURRENT_LIST_DIR}")

# We prepare the translations.qrc file and insert all available compiled translation files now.
set(QRC_ITEMS "")
foreach(TRANSLATION_FILE ${TRANSLATION_BIN_FILES})
	get_filename_component(FILENAME "${TRANSLATION_FILE}" NAME)
	set(QRC_ITEMS "${QRC_ITEMS}\n<file alias=\"${FILENAME}\">${TRANSLATION_FILE}</file>")
endforeach() 
configure_file("${CMAKE_CURRENT_LIST_DIR}/translations.qrc" "${CMAKE_BINARY_DIR}/translations.qrc" @ONLY)
QT5_ADD_RESOURCES(TRANSLATION_QRC "${CMAKE_BINARY_DIR}/translations.qrc")
