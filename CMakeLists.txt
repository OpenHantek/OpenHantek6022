# We need cmake 3.0 for Qt AUTORCC
cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
project(OpenHantekProject)

# Default build type
IF(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
   SET(CMAKE_BUILD_TYPE RelWithDebInfo)
ENDIF(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)

# Select flags
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall") # -Wfatal-errors
SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fno-rtti")
SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG -O0")
SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -DDEBUG -O0")

# C++11: Workaround for cmake < 3.1 with no target_compile_features support.
if (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

# Find external libraries
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/")

INCLUDE_DIRECTORIES(.)

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
