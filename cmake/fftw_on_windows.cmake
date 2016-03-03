if (CMAKE_SIZEOF_VOID_P EQUAL 4)
    message("Download FFTW for mingw32")
    if (NOT EXISTS "${CMAKE_BINARY_DIR}/fftw.zip")
        file(DOWNLOAD "ftp://ftp.fftw.org/pub/fftw/fftw-3.3.4-dll32.zip" "${CMAKE_BINARY_DIR}/fftw.zip" SHOW_PROGRESS)
    endif()
elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
    message("Download FFTW for mingw64")
    if (NOT EXISTS "${CMAKE_BINARY_DIR}/fftw.zip")
        file(DOWNLOAD "ftp://ftp.fftw.org/pub/fftw/fftw-3.3.4-dll64.zip" "${CMAKE_BINARY_DIR}/fftw.zip" SHOW_PROGRESS)
    endif()
else()
    message(FATAL_ERROR "Target architecture not known")
endif()

file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/fftw")

execute_process(
COMMAND ${CMAKE_COMMAND} -E tar xzf ${CMAKE_BINARY_DIR}/fftw.zip
WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/fftw"
)
target_link_libraries(${PROJECT_NAME} "${CMAKE_BINARY_DIR}/fftw/libfftw3-3.dll")
include_directories("${CMAKE_BINARY_DIR}/fftw")

file(COPY "${CMAKE_BINARY_DIR}/fftw/libfftw3-3.dll" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
