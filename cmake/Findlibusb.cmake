find_path(LIBUSB_INCLUDE_DIR
    NAMES
        libusb.h
    PATHS
        /usr/local/include
        /opt/local/include
        /usr/include
        /sw/include
    PATH_SUFFIXES
        libusb-1.0
)

if (LIBUSB_USE_STATIC_LIBS)
    set (LIBUSB_LIB_PREFIX "lib" CACHE INTERNAL "libusb library name prefix passed to find_library")
    set (LIBUSB_LIB_SUFFIX ".a" CACHE INTERNAL "libusb library name suffix passed to find_library")
else ()
    set (LIBUSB_LIB_PREFIX "" CACHE INTERNAL "libusb library name prefix passed to find_library")
    set (LIBUSB_LIB_SUFFIX "" CACHE INTERNAL "libusb library name suffix passed to find_library")
endif ()

find_library(LIBUSB_LIBRARY
    NAMES
        ${LIBUSB_LIB_PREFIX}usb-1.0${LIBUSB_LIB_SUFFIX} ${LIBUSB_LIB_PREFIX}usb${LIBUSB_LIB_SUFFIX}
    PATHS
        /usr/local/lib
        /opt/local/lib
        /usr/lib
        /lib64/
        /sw/lib
        /usr/lib/i386-linux-gnu/
        /usr/lib/x86_64-linux-gnu/
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(libusb REQUIRED_VARS LIBUSB_LIBRARY LIBUSB_INCLUDE_DIR)

if (LIBUSB_FOUND)
    set(LIBUSB_INCLUDE_DIRS ${LIBUSB_INCLUDE_DIR})
    set(LIBUSB_LIBRARIES ${LIBUSB_LIBRARY})
    mark_as_advanced(LIBUSB_INCLUDE_DIR LIBUSB_LIBRARY)
    if (NOT LIBUSB_FIND_QUIETLY)
      message(STATUS "Found libusb:")
	  message(STATUS " - Includes: ${LIBUSB_INCLUDE_DIRS}")
	  message(STATUS " - Libraries: ${LIBUSB_LIBRARIES}")
    endif (NOT LIBUSB_FIND_QUIETLY)
endif (LIBUSB_FOUND)
