# This file configures CPack. We setup a version number that contains
# the current git revision if git is found. A zip file is created on
# all platforms. Additionally an NSIS Installer exe is created on windows
# and a .sh installer file for linux.

find_package(Git QUIET)

execute_process(
    COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    RESULT_VARIABLE CMD_RESULT
    OUTPUT_VARIABLE VCS_REVISION
    OUTPUT_STRIP_TRAILING_WHITESPACE
    )

if(NOT DEFINED CMD_RESULT)
    message(WARNING "GIT executable not found. Make your PATH environment variable point to git")
    return()
else()
    execute_process(
        COMMAND ${GIT_EXECUTABLE} status
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        RESULT_VARIABLE CMD_RESULT
        OUTPUT_VARIABLE DESCRIBE_STATUS
        OUTPUT_STRIP_TRAILING_WHITESPACE
      )

    string(REPLACE "\n" " " DESCRIBE_STATUS ${DESCRIBE_STATUS})
    string(REPLACE "\r" " " DESCRIBE_STATUS ${DESCRIBE_STATUS})
    string(REPLACE "\rn" " " DESCRIBE_STATUS ${DESCRIBE_STATUS})
    string(REPLACE " " ";" DESCRIBE_STATUS ${DESCRIBE_STATUS})
    list(GET DESCRIBE_STATUS 2 VCS_BRANCH)

execute_process(
    COMMAND ${GIT_EXECUTABLE} config --get remote.origin.url
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    RESULT_VARIABLE CMD_RESULT
    OUTPUT_VARIABLE VCS_URL
    OUTPUT_STRIP_TRAILING_WHITESPACE
    )
endif()

string(TIMESTAMP DATE_VERSION "%d.%m.%Y")
string(TIMESTAMP CURRENT_TIME "%d.%m.%Y %H:%M")

if (UNIX)
    set(CPACK_GENERATOR ZIP STGZ)
	if (NOT APPLE)
		 set(CPACK_GENERATOR ${CPACK_GENERATOR} DEB)
	endif()
elseif(WIN32)
    set(CPACK_GENERATOR ZIP NSIS)
endif()

set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_VERSION "${DATE_VERSION}-${VCS_BRANCH}-${VCS_REVISION}")
set(CPACK_PACKAGE_CONTACT "contact@openhantek.org")
set(CPACK_PACKAGE_VENDOR "OpenHantek Community")
set(CPACK_PACKAGE_DESCRIPTION "Digitial Oscilloscope for Hantek USB DSO hardware.")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "OpenHantek is a free software for Hantek (Voltcraft/Darkwire/Protek/Acetech) USB DSOs based on HantekDSO. Build on ${CURRENT_TIME} from ${VCS_URL}. Branch ${VCS_BRANCH} has been used at commit ${VCS_REVISION}")
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/readme.md")
if (EXISTS "${CMAKE_SOURCE_DIR}/COPYING")
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/COPYING")
endif()
set(CPACK_DEBIAN_PACKAGE_SECTION "net")
IF ((MSVC AND CMAKE_GENERATOR MATCHES "Win64+") OR (CMAKE_SIZEOF_VOID_P EQUAL 8))
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
else()
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "i586")
endif()
set(CPACK_STRIP_FILES 1)

include(CMakeDetermineSystem)

set(CPACK_NSIS_EXECUTABLES_DIRECTORY ".")

set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY 0)
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CMAKE_SYSTEM_NAME}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY ".")
SET(CPACK_OUTPUT_FILE_PREFIX packages)

include(CPack)
set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION ".")
include(InstallRequiredSystemLibraries)

cpack_add_install_type(Full DISPLAY_NAME "All")

set(VERSION ${CPACK_PACKAGE_VERSION})