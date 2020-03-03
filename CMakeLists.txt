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

# Enable C++ standard library hardening -> cheap range checks for C++ arrays, vectors, and strings.
add_compile_options( -D_GLIBCXX_ASSERTIONS )

if(MSVC)
  add_compile_options(/W4)
else()
  add_compile_options(-Wall -Wextra -pedantic)
endif()

# enable extra feature(s)
if ( DEFINED HANTEK_AC )
    if( ${CMAKE_VERSION} VERSION_LESS "3.12.0" )
	   add_definitions( -DHANTEK_AC ) # deprecated
    else()
	   add_compile_definitions( HANTEK_AC ) # introduced with 3.12
    endif()
endif()

# show all compile definitions
#get_directory_property( DirDefs COMPILE_DEFINITIONS )
#message( "COMPILE_DEFINITIONS = ${DirDefs}" )

# Qt Widgets based Gui with OpenGL canvas
add_subdirectory(openhantek)

if (WIN32)
    install(FILES COPYING readme.md DESTINATION ".")
#else()
#    add_subdirectory(firmware EXCLUDE_FROM_ALL)
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
            DESTINATION "/usr/share/doc/openhantek" COMPONENT Runtime)
  else()
    message(WARNING "Could not find doc directory (/usr/share/doc/), skipping installation of user documentation.")
  endif()
endif()

# Add auxiliary files to the project, so that these files appear in VisualStudio/QtCreator
file(GLOB_RECURSE MDFILES "docs/*.md" "openhantek/*.md")
add_custom_target(readme SOURCES readme.md ${MDFILES})

# Add "cppcheck" command
add_custom_target(cppcheck COMMAND "cppcheck --enable=all -I \"${CMAKE_CURRENT_LIST_DIR}/openhantek/src\" -q ${SRC} --template=\"{file}:{line}: {severity}: {message}\"")

# Add "doc" target to build the documentation.
find_package(Doxygen QUIET)
if (DOXYGEN_FOUND)
    add_custom_target(doc
        COMMAND "${CMAKE_COMMAND} -E remove_directory html"
        COMMAND "${DOXYGEN_EXECUTABLE} Doxyfile" WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}")
endif()
