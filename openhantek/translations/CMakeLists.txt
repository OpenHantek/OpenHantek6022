set_directory_properties(PROPERTIES CLEAN_NO_CUSTOM 1)

find_package(Qt5LinguistTools REQUIRED)
if (NOT FOUND_Qt5LinguistTools)
    message(STATUS "Will not build translations! Please install Qt5LinguistTools")
    return()
endif()

file(GLOB TRANSLATION_FILES ${CMAKE_CURRENT_LIST_DIR}/*.ts)

qt5_create_translation(TRANSLATION_MESSAGES ${SRC} ${TRANSLATION_FILES})
qt5_add_translation(TRANSLATION_QM ${TRANSLATION_FILES})
add_custom_target(translations_update DEPENDS ${TRANSLATION_MESSAGES})
add_custom_target(translations DEPENDS ${TRANSLATION_QM})

# We prepare the translations.qrc file and insert all available compiled translation files now.
set(QRC_ITEMS "")
foreach(TRANSLATION_FILE ${TRANSLATION_MESSAGES})
	get_filename_component(FILENAME "${TRANSLATION_FILE}" NAME)
	set(QRC_ITEMS "${QRC_ITEMS}\n<file alias=\"${FILENAME}\">${TRANSLATION_FILE}</file>")
endforeach() 
configure_file("${CMAKE_CURRENT_LIST_DIR}/translations.qrc" "${CMAKE_BINARY_DIR}/translations.qrc" @ONLY)
