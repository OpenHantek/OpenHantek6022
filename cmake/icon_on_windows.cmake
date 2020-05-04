if (NOT WIN32)
    return()
endif()

set(iconname "${CMAKE_CURRENT_LIST_DIR}/OpenHantek.ico")

if (EXISTS "${iconname}")
    add_custom_command(TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy "${iconname}" $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMENT "Copy icons for ${PROJECT_NAME}"
    )
endif()
