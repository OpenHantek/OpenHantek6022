# This file configures CPack.
# A tgz, deb and rpm file is created for Linux (tgz and deb tested on debian stretch and buster).
# A tar.gz file is created for macOS (not tested).
# A zip file is created on Windows (not tested).

# create a changelog of the last 20 changes
set(ENV{LANG} "en_US")
execute_process(
    COMMAND ${GIT_EXECUTABLE} log -n 20 "--date=format:%a %b %d %Y" "--pretty=format:* %ad %aN <%aE> %h - %s"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    RESULT_VARIABLE CMD_RESULT
    OUTPUT_VARIABLE CHANGELOG
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
file(WRITE "${CMAKE_BINARY_DIR}/changelog" "${CHANGELOG}")

if (UNIX)
    execute_process(
        COMMAND uname -m
        WORKING_DIRECTORY "."
        OUTPUT_VARIABLE CPACK_ARCH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")
    set(CPACK_GENERATOR TGZ)

    if (CMAKE_SYSTEM_NAME MATCHES "Linux")
        set(CPACK_TARGET "")
        set(CPACK_GENERATOR ${CPACK_GENERATOR} DEB RPM)
        install(
            FILES utils/udev_rules/60-openhantek.rules
            DESTINATION lib/udev/rules.d
        )
    elseif(CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
        set(CPACK_TARGET "freebsd_")
        set(CPACK_PACKAGING_INSTALL_PREFIX "/usr/local")
        install(
            FILES utils/devd_rules_freebsd/openhantek.conf
            DESTINATION etc/devd
        )
    elseif(APPLE)
        set(CPACK_TARGET "osx_")
    endif()

    # install documentation
    FILE(GLOB PDF "docs/*.pdf")
    install(
        FILES CHANGELOG LICENSE README ${PDF}
        DESTINATION share/doc/openhantek
    )
    # install application starter and icons
    install(
        FILES utils/applications/OpenHantek.desktop
        DESTINATION share/applications
        )
    install(
        FILES openhantek/res/images/OpenHantek.png
        DESTINATION share/icons/hicolor/48x48/apps
    )
    install(
        FILES openhantek/res/images/OpenHantek.svg
        DESTINATION share/icons/hicolor/scalable/apps
    )

elseif(MSVC)
    set(CPACK_TARGET "msvc_")
    set(CPACK_GENERATOR ${CPACK_GENERATOR} ZIP)
    set(CPACK_ARCH "x64")
elseif(MINGW)
    set(CPACK_TARGET "mingw_")
    set(CPACK_GENERATOR ${CPACK_GENERATOR} ZIP)
    set(CPACK_ARCH "x64")
endif()

string(TOLOWER ${CMAKE_PROJECT_NAME} CPACK_PACKAGE_NAME)
set(CPACK_PACKAGE_VERSION "${PACKAGE_VERSION}")

message(STATUS "CPACK_GENERATOR: ${CPACK_GENERATOR}")
message(STATUS "CPACK_PACKAGE_NAME: ${CPACK_PACKAGE_NAME}")
message(STATUS "CPACK_PACKAGE_VERSION: ${CPACK_PACKAGE_VERSION}")
message(STATUS "CPACK_ARCH: ${CPACK_ARCH}")

set(CPACK_PACKAGE_CONTACT "contact@openhantek.org")
set(CPACK_PACKAGE_VENDOR "OpenHantek Community")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Digital oscilloscope software for Hantek DSO6022 USB hardware")
set(CPACK_PACKAGE_DESCRIPTION "OpenHantek is an oscilloscope software for\nVoltcraft/Darkwire/Protek/Acetech/Hantek USB devices")
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")
if (EXISTS "${CMAKE_SOURCE_DIR}/LICENSE")
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
endif()

set(CPACK_STRIP_FILES 1)

include(CMakeDetermineSystem)

# Linux DEB (tested on debian stable "bullseye")
# Architecture for package and file name are automatically detected
set(CPACK_DEBIAN_PACKAGE_SECTION "electronics")
# do not detect depencencies and versions automatically
# set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
# use deb stable packages without version explicitely to support also legacy installations
# local build uses Debian stable (currently bookworm)
# CI build (github actions) uses Ubuntu 22.04 LTS as long as Debian "bookworm" is "stable"
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6, libfftw3-double3, libglu1-mesa, libglx0, libopengl0, libqt5opengl5, libqt5printsupport5, libusb-1.0-0")
message( "-- Depends: ${CPACK_DEBIAN_PACKAGE_DEPENDS}" )

set(CPACK_DEBIAN_FILE_NAME "DEB-DEFAULT")

# Linux RPM (not tested on debian)
# Architecture for package and file name are automatically detected
set(CPACK_RPM_PACKAGE_RELOCATABLE NO)
set(CPACK_RPM_PACKAGE_LICENSE "GPLv2+")
set(CPACK_RPM_PACKAGE_DESCRIPTION ${CPACK_PACKAGE_DESCRIPTION})
# set(CPACK_RPM_CHANGELOG_FILE "${CMAKE_BINARY_DIR}/changelog")
set(CPACK_RPM_FILE_NAME "RPM-DEFAULT")

set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY 0)
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}_${CPACK_PACKAGE_VERSION}_${CPACK_TARGET}${CPACK_ARCH}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY ".")
SET(CPACK_OUTPUT_FILE_PREFIX packages)

include(CPack)
set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION ".")
include(InstallRequiredSystemLibraries)

cpack_add_install_type(Full DISPLAY_NAME "All")
