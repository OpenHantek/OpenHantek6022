# This file configures CPack. We setup a version number that contains
# the current git revision if git is found. A zip file is created on
# all platforms. Additionally an NSIS Installer exe is created on windows
# and a .sh installer file for linux.

find_package(Git QUIET)

if (GIT_EXECUTABLE AND EXISTS "${CMAKE_SOURCE_DIR}/.git")
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        RESULT_VARIABLE CMD_RESULT
        OUTPUT_VARIABLE VCS_REVISION
        OUTPUT_STRIP_TRAILING_WHITESPACE
        )
endif()

if(NOT DEFINED CMD_RESULT)
    set(VCS_BRANCH "master")
    set(VCS_REVISION "na")
else()
    execute_process(
        COMMAND ${GIT_EXECUTABLE} status
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        RESULT_VARIABLE CMD_RESULT
        OUTPUT_VARIABLE DESCRIBE_STATUS
        OUTPUT_STRIP_TRAILING_WHITESPACE
      )

    string(REPLACE "\n" " " DESCRIBE_STATUS ${DESCRIBE_STATUS})
    string(REPLACE "\r" " " DESCRIBE_STATUS ${DESCRIBE_STATUS})
    string(REPLACE "\rn" " " DESCRIBE_STATUS ${DESCRIBE_STATUS})
    string(REPLACE " " ";" DESCRIBE_STATUS ${DESCRIBE_STATUS})
    list(GET DESCRIBE_STATUS 2 VCS_BRANCH)

    message(STATUS "Version: ${VCS_BRANCH}/${VCS_REVISION}")

    execute_process(
        COMMAND ${GIT_EXECUTABLE} config --get remote.origin.url
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        RESULT_VARIABLE CMD_RESULT
        OUTPUT_VARIABLE VCS_URL
        OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        
    set(ENV{LANG} "en_US")
    if(GIT_VERSION_STRING VERSION_LESS 2.6)
        set(CHANGELOG "")
    else()
        execute_process(
            COMMAND ${GIT_EXECUTABLE} log -n 10 "--date=format:%a %b %d %Y" "--pretty=format:* %ad %aN <%aE> %h - %s"
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            RESULT_VARIABLE CMD_RESULT
            OUTPUT_VARIABLE CHANGELOG
            OUTPUT_STRIP_TRAILING_WHITESPACE
            )
    endif()
    file(WRITE "${CMAKE_BINARY_DIR}/changelog" "${CHANGELOG}")
endif()

string(TIMESTAMP DATE_VERSION "%d.%m.%Y")
string(TIMESTAMP CURRENT_TIME "%d.%m.%Y %H:%M")

if (UNIX)
    set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")
    set(CPACK_GENERATOR STGZ)
	if (NOT APPLE)
        set(CPACK_GENERATOR ${CPACK_GENERATOR} ZIP)
        find_program(LSB_RELEASE lsb_release)
        execute_process(COMMAND ${LSB_RELEASE} -is
            OUTPUT_VARIABLE LSB_RELEASE_ID_SHORT
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        if (LSB_RELEASE_ID_SHORT MATCHES "Ubuntu")
            set(CPACK_GENERATOR ${CPACK_GENERATOR} DEB)
        else()
            set(CPACK_GENERATOR ${CPACK_GENERATOR} RPM)
        endif()
	endif()
elseif(WIN32)
    set(CPACK_GENERATOR NSIS)
endif()

set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
string(TOLOWER ${CPACK_PACKAGE_NAME} CPACK_PACKAGE_NAME)
set(CPACK_PACKAGE_VERSION "${DATE_VERSION}_${VCS_REVISION}")
set(CPACK_PACKAGE_CONTACT "contact@openhantek.org")
set(CPACK_PACKAGE_VENDOR "OpenHantek Community")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Digital oscilloscope software for Hantek USB hardware")
set(CPACK_PACKAGE_DESCRIPTION "OpenHantek is an oscilloscope software for\nVoltcraft/Darkwire/Protek/Acetech/Hantek USB devices")
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/readme.md")
if (EXISTS "${CMAKE_SOURCE_DIR}/COPYING")
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/COPYING")
endif()

# Linux DEB+RPM 
set(CPACK_DEBIAN_PACKAGE_SECTION "net")
set(CPACK_ARCH)
IF ((MSVC AND CMAKE_GENERATOR MATCHES "Win64+") OR (CMAKE_SIZEOF_VOID_P EQUAL 8))
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
    set(CPACK_RPM_PACKAGE_ARCHITECTURE "x86_64")
    set(CPACK_ARCH "x86_64")
else()
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "i586")
    set(CPACK_RPM_PACKAGE_ARCHITECTURE "i586")
    set(CPACK_ARCH "x86")
endif()
set(CPACK_STRIP_FILES 1)

include(CMakeDetermineSystem)

# Linux RPM
set(CPACK_RPM_PACKAGE_RELOCATABLE NO)
set(CPACK_RPM_PACKAGE_LICENSE "GPLv2+")
set(CPACK_RPM_PACKAGE_DESCRIPTION ${CPACK_PACKAGE_DESCRIPTION})
set(CPACK_RPM_PACKAGE_REQUIRES "qt5-qtbase-gui%{?_isa} >= 5.4, qt5-qttranslations%{?_isa}")
set(CPACK_RPM_CHANGELOG_FILE "${CMAKE_BINARY_DIR}/changelog")

set(CPACK_NSIS_EXECUTABLES_DIRECTORY ".")

set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY 0)
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-1.${CPACK_ARCH}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY ".")
SET(CPACK_OUTPUT_FILE_PREFIX packages)

include(CPack)
set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION ".")
include(InstallRequiredSystemLibraries)

cpack_add_install_type(Full DISPLAY_NAME "All")

set(VERSION ${CPACK_PACKAGE_VERSION})
