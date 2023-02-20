# set source and destination directory names
set(OH_DOCS_DIR "${CMAKE_CURRENT_LIST_DIR}/../docs")
set(DOCUMENTS_DIR "documents")

# set file names for doc files
set(USER_MANUAL "${OH_DOCS_DIR}/OpenHantek6022_User_Manual.pdf")
set(AC_MOD "${OH_DOCS_DIR}/HANTEK6022_AC_Modification.pdf")
set(FG_MOD "${OH_DOCS_DIR}/HANTEK6022_Frequency_Generator_Modification.pdf")

# execute commands
add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${DOCUMENTS_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${USER_MANUAL}" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${DOCUMENTS_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${AC_MOD}" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${DOCUMENTS_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${FG_MOD}" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${DOCUMENTS_DIR}"
    COMMENT "Copy documentation files for ${PROJECT_NAME}"
)
