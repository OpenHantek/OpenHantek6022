if (NOT WIN32)
        return()
endif()

# where are the documents (*.pdf) and where to copy
set(SRC_DOCS_DIR "${CMAKE_CURRENT_LIST_DIR}/../docs")
set(TARGET_DOCUMENTS "documents")

# execute commands
add_custom_command(TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${TARGET_DOCUMENTS}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SRC_DOCS_DIR}/OpenHantek6022_User_Manual.pdf" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${TARGET_DOCUMENTS}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SRC_DOCS_DIR}/HANTEK6022_AC_Modification.pdf" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${TARGET_DOCUMENTS}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SRC_DOCS_DIR}/HANTEK6022_Frequency_Generator_Modification.pdf" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${TARGET_DOCUMENTS}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SRC_DOCS_DIR}/OpenHantek6022_zadig_Win10.pdf" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${TARGET_DOCUMENTS}"
        COMMENT "Copy manual and documentation pdf files into ${PROJECT_NAME}/${TARGET_DOCUMENTS}"
)
