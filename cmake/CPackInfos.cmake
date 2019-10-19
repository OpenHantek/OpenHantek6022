# This file configures CPack. 
# A tgz, deb and rpm file is created for linux (tgz and deb tested on debian stretch and buster).
# A tgz file is created for osx (not tested).
# A zip file and an NSIS Installer exe is created on windows (not tested).

find_package(Git QUIET)

if (GIT_EXECUTABLE AND EXISTS "${CMAKE_SOURCE_DIR}/.git")
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        RESULT_VARIABLE CMD_RESULT
        # OUTPUT_VARIABLE VCS_REVISION
        OUTPUT_STRIP_TRAILING_WHITESPACE
        )
endif()

if(NOT DEFINED CMD_RESULT)
    set(VCS_BRANCH "master")
    set(GIT_COMMIT_HASH "1")
    # set(VCS_REVISION "na")
else()
    execute_process(
        COMMAND git log -1 --format=%h
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_COMMIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

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

    message(STATUS "Branch: ${VCS_BRANCH}") # /${VCS_REVISION}")

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

string(TIMESTAMP DATE_VERSION "%Y%m%d")
string(TIMESTAMP CURRENT_TIME "%Y%m%d_%H:%M")


if (UNIX)
    execute_process(
        COMMAND uname -m
        WORKING_DIRECTORY "."
        OUTPUT_VARIABLE CPACK_ARCH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")
    set(CPACK_GENERATOR TGZ)
    if (NOT APPLE)
        set(CPACK_TARGET "")
        set(CPACK_GENERATOR ${CPACK_GENERATOR} DEB RPM)
    else()
        set(CPACK_TARGET "osx_")
    endif()
elseif(WIN32)
    set(CPACK_TARGET "win_")
    set(CPACK_GENERATOR ${CPACK_GENERATOR} ZIP NSIS)
    if ((MSVC AND CMAKE_GENERATOR MATCHES "Win64+") OR (CMAKE_SIZEOF_VOID_P EQUAL 8))
        set(CPACK_ARCH "amd64")
    else()
        set(CPACK_ARCH "x86")
    endif()
endif()

message(STATUS "Architecture: ${CPACK_ARCH}")

set(CPACK_PACKAGE_NAME "openhantek")
string(TOLOWER ${CPACK_PACKAGE_NAME} CPACK_PACKAGE_NAME)
set(CPACK_PACKAGE_VERSION "${DATE_VERSION}-${GIT_COMMIT_HASH}")
# set(CPACK_PACKAGE_VERSION "${DATE_VERSION}_${VCS_REVISION}")
# set(CPACK_PACKAGE_VERSION "${DATE_VERSION}")
set(CPACK_PACKAGE_CONTACT "contact@openhantek.org")
set(CPACK_PACKAGE_VENDOR "OpenHantek Community")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Digital oscilloscope software for Hantek DSO6022 USB hardware")
set(CPACK_PACKAGE_DESCRIPTION "OpenHantek is an oscilloscope software for\nVoltcraft/Darkwire/Protek/Acetech/Hantek USB devices")
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/readme.md")
if (EXISTS "${CMAKE_SOURCE_DIR}/COPYING")
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/COPYING")
endif()

set(CPACK_STRIP_FILES 1)

include(CMakeDetermineSystem)

# Linux DEB (tested on debian stretch and buster)
# Architecture for package and file name are automatically detected
set(CPACK_DEBIAN_PACKAGE_SECTION "electronics")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libqt5core5a, libqt5opengl5, libopengl0, libusb-1.0-0, libfftw3-3")
set(CPACK_DEBIAN_FILE_NAME "DEB-DEFAULT")

# Linux RPM (not tested on debian)
# Architecture for package and file name are automatically detected
set(CPACK_RPM_PACKAGE_RELOCATABLE NO)
set(CPACK_RPM_PACKAGE_LICENSE "GPLv2+")
set(CPACK_RPM_PACKAGE_DESCRIPTION ${CPACK_PACKAGE_DESCRIPTION})
set(CPACK_RPM_PACKAGE_REQUIRES "qt5-qtbase-gui%{?_isa} >= 5.4, qt5-qttranslations%{?_isa}")
set(CPACK_RPM_CHANGELOG_FILE "${CMAKE_BINARY_DIR}/changelog")
set(CPACK_RPM_FILE_NAME "RPM-DEFAULT")
set(CPACK_NSIS_EXECUTABLES_DIRECTORY ".")

set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY 0)
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}_${CPACK_PACKAGE_VERSION}-1_${CPACK_TARGET}${CPACK_ARCH}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY ".")
SET(CPACK_OUTPUT_FILE_PREFIX packages)

include(CPack)
set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION ".")
include(InstallRequiredSystemLibraries)

cpack_add_install_type(Full DISPLAY_NAME "All")

set(VERSION ${CPACK_PACKAGE_VERSION})
