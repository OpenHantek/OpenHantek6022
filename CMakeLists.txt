cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(OpenHantek)

set(OpenGL_GL_PREFERENCE GLVND)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Default build type
IF(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
   SET(CMAKE_BUILD_TYPE RelWithDebInfo)
ENDIF(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)

# Find external libraries
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/")

INCLUDE_DIRECTORIES(".")

find_package(Git)

if (GIT_EXECUTABLE AND EXISTS "${CMAKE_SOURCE_DIR}/.git")
    execute_process(
        # delete the unstable tag ...
        # COMMAND ${GIT_EXECUTABLE} tag -d unstable
        # ... to get the real versioned tag
        COMMAND ${GIT_EXECUTABLE} describe --dirty
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
endif()
message(STATUS "VERSION: ${VERSION}")

# Use CPack to make tgz/deb/rpm/zip installer packages
include(cmake/CPackInfos.cmake)

add_compile_options(-Wall -Wextra -pedantic)

# Silence nasty warnings "parameter passing for argument of type ... changed in GCC 7.1"
if(CMAKE_COMPILER_IS_GNUCXX)
  add_compile_options(-Wno-psabi)
endif()

# enable extra feature(s)
if( ${CMAKE_VERSION} VERSION_LESS "3.12.0" ) # deprecated, but still used in older Ubuntu releases
    # Enable C++ standard library hardening -> cheap range checks for C++ arrays, vectors, and strings.
    add_definitions( -D_GLIBCXX_ASSERTIONS )
    add_definitions( -D_USE_MATH_DEFINES )
    if ( DEFINED VERSION )
        add_definitions( -DVERSION="${VERSION}" )
    endif()
else() # 'add_compile_definitions()' was introduced with CMake 3.12
    add_compile_definitions( _GLIBCXX_ASSERTIONS )
    add_compile_definitions( _USE_MATH_DEFINES )
    if ( DEFINED VERSION )
        add_compile_definitions( VERSION="${VERSION}" )
    endif()
endif()

# show all compile options and definitions
get_directory_property( CompOpts COMPILE_OPTIONS )
message( "-- COMPILE_OPTIONS: ${CompOpts}" )
get_directory_property( CompDefs COMPILE_DEFINITIONS )
message( "-- COMPILE_DEFINITIONS: ${CompDefs}" )

# Qt Widgets based Gui with OpenGL canvas
add_subdirectory(openhantek)

if (WIN32)
    install(
        FILES CHANGELOG LICENSE README
        DESTINATION "."
    )
endif()

# Add auxiliary files to the project, so that these files appear in VisualStudio/QtCreator
file(GLOB_RECURSE MDFILES "docs/*.md" "openhantek/*.md")
add_custom_target(
    readme
    SOURCES CHANGELOG LICENSE README README.md ${MDFILES}
)

