if (NOT WIN32)
    return()
endif()

macro( CheckExitCodeAndExitIfError MSG)
  if(NOT ${ExitCode} EQUAL 0)
    message(FATAL_ERROR "Failed: ${MSG}")
    return(${ExitCode})
  endif()
endmacro( CheckExitCodeAndExitIfError )

if (CMAKE_SIZEOF_VOID_P EQUAL 4)
    message("Download FFTW for mingw32")
    if (NOT EXISTS "${CMAKE_BINARY_DIR}/fftw.zip")
        file(DOWNLOAD "ftp://ftp.fftw.org/pub/fftw/fftw-3.3.5-dll32.zip" "${CMAKE_BINARY_DIR}/fftw.zip" SHOW_PROGRESS)
    endif()
elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
    message("Download FFTW for mingw64")
    if (NOT EXISTS "${CMAKE_BINARY_DIR}/fftw.zip")
        file(DOWNLOAD "ftp://ftp.fftw.org/pub/fftw/fftw-3.3.5-dll64.zip" "${CMAKE_BINARY_DIR}/fftw.zip" SHOW_PROGRESS)
    endif()
else()
    message(FATAL_ERROR "Target architecture not known")
endif()

file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/fftw")

execute_process(
    COMMAND ${CMAKE_COMMAND} -E tar xzf ${CMAKE_BINARY_DIR}/fftw.zip
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/fftw"
    RESULT_VARIABLE ExitCode
)
CheckExitCodeAndExitIfError("tar")

get_filename_component(_vs_bin_path "${CMAKE_LINKER}" DIRECTORY)

execute_process(
    COMMAND "${_vs_bin_path}/lib.exe" /machine:x64 /def:${CMAKE_BINARY_DIR}/fftw/libfftw3-3.def /out:${CMAKE_BINARY_DIR}/fftw/libfftw3-3.lib
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/fftw"
    RESULT_VARIABLE ExitCode)
CheckExitCodeAndExitIfError("lib")

target_link_libraries(${PROJECT_NAME} "${CMAKE_BINARY_DIR}/fftw/libfftw3-3.lib")
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_BINARY_DIR}/fftw")

file(COPY "${CMAKE_BINARY_DIR}/fftw/fftw3.h" DESTINATION "${CMAKE_SOURCE_DIR}/src")

add_custom_command(TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_BINARY_DIR}/fftw/libfftw3-3.dll" $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMENT "Copy fftw3 dlls for ${PROJECT_NAME}"
)

