###################################
#
# Add more languages here
#
set(TS_FILES translations/openhantek_de.ts translations/openhantek_es.ts translations/openhantek_fr.ts translations/openhantek_it.ts translations/openhantek_pl.ts translations/openhantek_pt.ts translations/openhantek_ru.ts translations/openhantek_sv.ts translations/openhantek_zh.ts)
#
###################################

# Find the Qt linguist tool
find_package(Qt5 REQUIRED COMPONENTS LinguistTools)

# defines files with translatable strings.
set(INPUT ${SRC} ${HEADERS} ${UI})

# prepares 'lupdate' to update ts files and also 'lcreate' to build qm files.
qt5_create_translation(QM_FILES ${INPUT} ${TS_FILES})

# prepare the translations.qrc file and insert all available compiled translation files now.
set(QRC_ITEMS "")
foreach(QM_FILE ${QM_FILES})
        get_filename_component(FILENAME "${QM_FILE}" NAME)
        set(QRC_ITEMS "${QRC_ITEMS}\n<file alias=\"${FILENAME}\">${QM_FILE}</file>")
endforeach()
configure_file("${CMAKE_CURRENT_LIST_DIR}/translations.qrc.template" "${CMAKE_BINARY_DIR}/translations.qrc" @ONLY)

QT5_ADD_RESOURCES(TRANSLATION_QRC "${CMAKE_BINARY_DIR}/translations.qrc")
