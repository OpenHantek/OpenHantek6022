project(OpenHantek CXX C)

find_package(Qt5Widgets REQUIRED)
find_package(Qt5PrintSupport REQUIRED)
find_package(Qt5OpenGL REQUIRED)
find_package(OpenGL)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# include directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
include_directories(src/ src/hantek)

# collect sources and other files
file(GLOB_RECURSE SRC "src/*.cpp")
file(GLOB_RECURSE HEADERS "src/*.h")
file(GLOB_RECURSE QRC "res/*.qrc")

add_subdirectory(translations)
add_subdirectory(res)

add_definitions(-DVERSION="${CPACK_PACKAGE_VERSION}")

# make executable
add_executable(${PROJECT_NAME} ${SRC} ${HEADERS} ${QRC} ${TRANSLATION_BIN_FILES} ${TRANSLATION_QRC})
target_link_libraries(${PROJECT_NAME} Qt5::Widgets Qt5::PrintSupport Qt5::OpenGL ${OPENGL_LIBRARIES} )
target_compile_features(${PROJECT_NAME} PRIVATE cxx_range_for)
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE "/W4" "/wd4251" "/wd4127" "/wd4275" "/nologo" "/J" "/Zi")
    target_compile_options(${PROJECT_NAME} PRIVATE "$<$<CONFIG:DEBUG>:/MDd>")
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wno-long-long -pedantic)
    target_compile_options(${PROJECT_NAME} PRIVATE "$<$<CONFIG:DEBUG>:-DDEBUG>")
    target_compile_options(${PROJECT_NAME} PRIVATE "$<$<CONFIG:DEBUG>:-O0>")
    target_compile_options(${PROJECT_NAME} PRIVATE "$<$<CONFIG:RELEASE>:-fno-rtti>")
endif()
    

if (UNIX)
    find_package(libusb REQUIRED)
    target_link_libraries(${PROJECT_NAME} ${LIBUSB_LIBRARIES})

    find_package(Threads REQUIRED)
    target_link_libraries(${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT})

    find_package(FFTW REQUIRED)
    target_link_libraries(${PROJECT_NAME} ${FFTW_LIBRARIES})
elseif(WIN32)
    include(../cmake/fftw_on_windows.cmake)
    include(../cmake/libusb_on_windows.cmake)
endif()

# install commands
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION "bin")

include(../cmake/copy_qt5_dlls_to_bin_dir.cmake)
