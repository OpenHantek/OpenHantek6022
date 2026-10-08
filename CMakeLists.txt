cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

# Release version. It is also the fallback when there is no usable git
# description: a source tarball has no .git directory, so the `git describe`
# block below never runs. Without a fallback PACKAGE_VERSION stays empty and
# CPackDeb aborts with "Debian package requires a package version"
# (OpenHantek/OpenHantek6022#457). Keep it in sync with the release tag.
project(OpenHantek VERSION 3.4.1)

find_package(Git)

# Overridden by `git describe` below whenever that succeeds.
set( VERSION "${PROJECT_VERSION}" )
set( PACKAGE_VERSION "${PROJECT_VERSION}" )

if (GIT_EXECUTABLE AND EXISTS "${CMAKE_SOURCE_DIR}/.git")
    # show modified files
    execute_process(
        COMMAND ${GIT_EXECUTABLE} status --short
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
    # get the git version
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --tags --long --dirty
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    # devdrop or release? (an empty VERSION means `git describe` failed above)
    string(FIND "${VERSION}" "devdrop-0-g" DEVDROP_POS)
    string(FIND "${VERSION}" "-0-g" ZERO_POS)
    if( ${DEVDROP_POS} EQUAL 0 ) # "x.y.z-p-gxxxxxxx(-dirty)-devdrop"
        execute_process(
            COMMAND ${GIT_EXECUTABLE} describe --long --dirty
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE VERSION
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        set( VERSION "${VERSION}-devdrop" )
    elseif( ${ZERO_POS} GREATER 0 ) # x.y.z(-dirty)
        # message(STATUS "TAG")
        execute_process(
            COMMAND ${GIT_EXECUTABLE} describe --dirty
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE VERSION
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
    else() # "x.y.z-p-gxxxxxxx(-dirty)"
        execute_process(
            COMMAND ${GIT_EXECUTABLE} describe --long --dirty
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE VERSION
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
    endif()
    if( VERSION )
        set( PACKAGE_VERSION "${VERSION}" )
    endif()

    # Re-run configure automatically when the git state changes. Otherwise VERSION is
    # baked into the compile definitions at configure time and `--version` keeps
    # reporting the commit that was checked out when the build tree was created,
    # no matter how often the user commits or switches branches afterwards.
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --absolute-git-dir
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE OH_GIT_DIR
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE OH_GIT_DIR_RESULT
    )
    if( OH_GIT_DIR_RESULT EQUAL 0 )
        foreach( OH_GIT_STATE_FILE HEAD index packed-refs )
            if( EXISTS "${OH_GIT_DIR}/${OH_GIT_STATE_FILE}" )
                set_property( DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
                              "${OH_GIT_DIR}/${OH_GIT_STATE_FILE}" )
            endif()
        endforeach()
    endif()
endif()

message(STATUS "PACKAGE_VERSION: ${PACKAGE_VERSION}")

set(OpenGL_GL_PREFERENCE GLVND)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Default build type
IF(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
   SET(CMAKE_BUILD_TYPE RelWithDebInfo)
ENDIF(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)

# Find external libraries
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/")

INCLUDE_DIRECTORIES(".")

if(MSVC)
  # NOMINMAX to avoid clash with std::min or std::max due to silly macros min and max
  add_compile_options(/W4 /D_CRT_SECURE_NO_WARNINGS /DNOMINMAX)
else()
  add_compile_options(-Wall -Wextra -pedantic)
endif()


# Silence nasty warnings "parameter passing for argument of type ... changed in GCC 7.1"
if(CMAKE_COMPILER_IS_GNUCXX)
  add_compile_options(-Wno-psabi)
endif()

# enable extra feature(s)
# 'add_compile_definitions()' was introduced with CMake 3.12
add_compile_definitions( _GLIBCXX_ASSERTIONS )
add_compile_definitions( _USE_MATH_DEFINES )
if ( VERSION )
    add_compile_definitions( VERSION="${VERSION}" )
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

# Use CPack to make tgz/deb/rpm/zip installer packages
include(cmake/CPackInfos.cmake)

