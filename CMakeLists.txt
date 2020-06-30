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

# Use CPack to make deb/rpm/zip/exe installer packages
include(cmake/CPackInfos.cmake)

if(MSVC)
  add_compile_options(/W4 -D_CRT_SECURE_NO_WARNINGS)
else()
  add_compile_options(-Wall -Wextra -pedantic)
endif()

# Silence nasty warnings "parameter passing for argument of type ... changed in GCC 7.1"
if(CMAKE_COMPILER_IS_GNUCXX)
  add_compile_options(-Wno-psabi)
endif()

# enable extra feature(s)
if( ${CMAKE_VERSION} VERSION_LESS "3.12.0" ) # deprecated, but still used in older Ubuntu releases
    # Enable C++ standard library hardening -> cheap range checks for C++ arrays, vectors, and strings.
    add_definitions( -D_GLIBCXX_ASSERTIONS )
    add_definitions( -D_USE_MATH_DEFINES )
    if ( DEFINED HANTEK_AC )
        add_definitions( -DHANTEK_AC )
    endif()
else() # 'add_compile_definitions()' was introduced with CMake 3.12
    add_compile_definitions( _GLIBCXX_ASSERTIONS )
    add_compile_definitions( _USE_MATH_DEFINES )
    if ( DEFINED HANTEK_AC )
        add_compile_definitions( HANTEK_AC )
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
    install(FILES COPYING readme.md DESTINATION ".")
endif()

if("${CMAKE_SYSTEM}" MATCHES "Linux")
  if(EXISTS "/lib/udev/rules.d/")
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/utils/udev_rules/60-hantek.rules"
      DESTINATION "/lib/udev/rules.d/" COMPONENT Runtime)
  else()
    message(WARNING "Could not find udev rules directory (/lib/udev/rules.d/), skipping installation of udev rules.")
  endif()
  if(EXISTS "/usr/share/applications/")
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/utils/applications/OpenHantek.desktop"
      DESTINATION "/usr/share/applications/" COMPONENT Runtime)
  else()
    message(WARNING "Could not find applications directory (/usr/share/applications/), skipping installation of desktop file.")
  endif()
  if(EXISTS "/usr/share/icons/hicolor/48x48/apps/")
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/openhantek/res/images/OpenHantek.png"
      DESTINATION /usr/share/icons/hicolor/48x48/apps/ COMPONENT Runtime)
  else()
    message(WARNING "Could not find icons directory (/usr/share/icons/hicolor/48x48/apps/), skipping installation of icon.")
  endif()
  if(EXISTS "/usr/share/icons/hicolor/scalable/apps/")
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/openhantek/res/images/OpenHantek.svg"
      DESTINATION "/usr/share/icons/hicolor/scalable/apps/" COMPONENT Runtime)
  else()
    message(WARNING "Could not find icons directory (/usr/share/icons/hicolor/scalable/apps/), skipping installation of icon.")
  endif()
  if(EXISTS "/usr/share/doc/")
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/CHANGELOG"
                  "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE"
                  "${CMAKE_CURRENT_SOURCE_DIR}/docs/OpenHantek6022_User_Manual.pdf"
                  "${CMAKE_CURRENT_SOURCE_DIR}/docs/HANTEK6022_AC_Modification.pdf"
            DESTINATION "/usr/share/doc/openhantek" COMPONENT Runtime)
  else()
    message(WARNING "Could not find doc directory (/usr/share/doc/), skipping installation of user documentation.")
  endif()
endif()

# Add auxiliary files to the project, so that these files appear in VisualStudio/QtCreator
file(GLOB_RECURSE MDFILES "docs/*.md" "openhantek/*.md")
add_custom_target(readme SOURCES readme.md ${MDFILES})

# Add "doc" target to build the documentation.
find_package(Doxygen QUIET)
if (DOXYGEN_FOUND)
    add_custom_target(doc
        COMMAND "${CMAKE_COMMAND} -E remove_directory html"
        COMMAND "${DOXYGEN_EXECUTABLE} Doxyfile" WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}")
endif()
