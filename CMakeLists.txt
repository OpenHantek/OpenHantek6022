cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(OpenHantekProject)

# Default build type
IF(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
   SET(CMAKE_BUILD_TYPE RelWithDebInfo)
ENDIF(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)

# Find external libraries
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/")

INCLUDE_DIRECTORIES("." "/usr/local/include")

# Qt Widgets based Gui with OpenGL canvas
add_subdirectory(openhantek)

# Use CPack to make deb/rpm/zip/exe installer packages
include(cmake/CPackInfos.cmake)

if (WIN32)
    install(FILES ChangeLog COPYING readme.md DESTINATION ".")
endif()

# Add auxiliary files to the project, so that these files appear in VisualStudio/QtCreator
add_custom_target(readme SOURCES readme.md)

# Add "cppcheck" command
add_custom_target(cppcheck COMMAND "cppcheck --enable=all -I \"${CMAKE_CURRENT_LIST_DIR}/libusbDSO\" -I \"${CMAKE_CURRENT_LIST_DIR}/libPostprocessingDSO\" -I \"${CMAKE_CURRENT_LIST_DIR}/openhantek/src\" -q ${SRC} --template=\"{file}:{line}: {severity}: {message}\"")

# Add "doc" target to build the documentation.
find_package(Doxygen QUIET)
if (DOXYGEN_FOUND)
    add_custom_target(doc
        COMMAND "${CMAKE_COMMAND} -E remove_directory html"
        COMMAND "${DOXYGEN_EXECUTABLE} Doxyfile" WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}")
endif()
