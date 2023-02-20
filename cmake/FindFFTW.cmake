# - Find the FFTW library
#
# Usage:
#   find_package(FFTW [REQUIRED] [QUIET] )
#
# It sets the following variables:
#   FFTW_FOUND					... true if fftw is found on the system
#   FFTW_LIBRARIES				... full path to fftw library
#   FFTW_INCLUDES				... fftw include directory
#
# The following variables will be checked by the function
#   FFTW_USE_STATIC_LIBS		... if true, only static libraries are found
#   FFTW_LIBRARIES				... fftw library to use
#   FFTW_INCLUDE_DIRS			... fftw include directory
#

if (FFTW_LIBRARIES AND FFTW_INCLUDE_DIRS)
  # in cache already
  set(FFTW_FOUND TRUE)
else (FFTW_LIBRARIES AND FFTW_INCLUDE_DIRS)

if (FFTW_USE_STATIC_LIBS)
  set (LIBFFTW_LIB_SUFFIX ".a" CACHE INTERNAL "libfftw3 library name suffix passed to find_library")
endif (FFTW_USE_STATIC_LIBS)

  find_path(FFTW_INCLUDE_DIR
    NAMES
	fftw3.h
    PATHS
      /usr/include
      /usr/local/include
      /opt/local/include
      /sw/include
  )

  find_library(FFTW_LIBRARY
    NAMES
      libfftw3${LIBFFTW_LIB_SUFFIX}
      fftw3
    PATHS
      /usr/lib
      /usr/local/lib
      /opt/local/lib
      /sw/lib
  )

  set(FFTW_INCLUDE_DIRS
    ${FFTW_INCLUDE_DIR}
  )
  set(FFTW_LIBRARIES
    ${FFTW_LIBRARY}
)

  if (FFTW_INCLUDE_DIRS AND FFTW_LIBRARIES)
     set(FFTW_FOUND TRUE)
  endif (FFTW_INCLUDE_DIRS AND FFTW_LIBRARIES)

  if (FFTW_FOUND)
    if (NOT FFTW_FIND_QUIETLY)
      message(STATUS "Found libfftw3:")
	  message(STATUS " - Includes: ${FFTW_INCLUDE_DIRS}")
	  message(STATUS " - Libraries: ${FFTW_LIBRARIES}")
    endif (NOT FFTW_FIND_QUIETLY)
  else (FFTW_FOUND)
    if (FFTW_FIND_REQUIRED)
      message(FATAL_ERROR "Could not find libfftw3")
    endif (FFTW_FIND_REQUIRED)
  endif (FFTW_FOUND)

  # show the FFTW_INCLUDE_DIRS and FFTW_LIBRARIES variables only in the advanced view
  mark_as_advanced(FFTW_INCLUDE_DIRS FFTW_LIBRARIES)

endif (FFTW_LIBRARIES AND FFTW_INCLUDE_DIRS)
