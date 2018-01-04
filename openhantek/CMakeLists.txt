project(OpenHantek CXX)

find_package(Qt5Widgets REQUIRED)
find_package(Qt5PrintSupport REQUIRED)
find_package(Qt5OpenGL REQUIRED)
find_package(OpenGL)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

if (Qt5Widgets_VERSION VERSION_LESS 5.4.0)
    message(FATAL_ERROR "Minimum supported Qt5 version is 5.4.0!")
endif()

# include directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
include_directories(src/ src/hantekdso src/widgets src/docks src/configdialog)

# collect sources and other files
file(GLOB_RECURSE SRC "src/*.cpp")
file(GLOB_RECURSE HEADERS "src/*.h")
file(GLOB_RECURSE UI "src/*.ui")
file(GLOB_RECURSE QRC "res/*.qrc")

add_custom_target(format SOURCES ".clang-format"
    COMMAND "clang-format" "-style=file" "-i" "-sort-includes" ${SRC} ${HEADERS})

add_subdirectory(translations)

add_definitions(-DVERSION="${CPACK_PACKAGE_VERSION}")

# make executable
add_executable(${PROJECT_NAME} ${SRC} ${HEADERS} ${UI} ${QRC} ${TRANSLATION_BIN_FILES} ${TRANSLATION_QRC})
target_link_libraries(${PROJECT_NAME} Qt5::Widgets Qt5::PrintSupport Qt5::OpenGL ${OPENGL_LIBRARIES} )
target_compile_features(${PROJECT_NAME} PRIVATE cxx_range_for)
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE "/W4" "/wd4251" "/wd4127" "/wd4275" "/wd4200" "/nologo" "/J" "/Zi")
    target_compile_options(${PROJECT_NAME} PRIVATE "$<$<CONFIG:DEBUG>:/MDd>")
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wno-long-long -pedantic)
    target_compile_options(${PROJECT_NAME} PRIVATE "$<$<CONFIG:DEBUG>:-DDEBUG>")
    target_compile_options(${PROJECT_NAME} PRIVATE "$<$<CONFIG:DEBUG>:-O0>")
    target_compile_options(${PROJECT_NAME} PRIVATE "$<$<CONFIG:RELEASE>:-fno-rtti>")
endif()

if(WIN32)
    include(../cmake/fftw_on_windows.cmake)
    include(../cmake/libusb_on_windows.cmake)
else()
    find_package(libusb REQUIRED)
    target_include_directories(${PROJECT_NAME} PRIVATE ${LIBUSB_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} ${LIBUSB_LIBRARIES})

    find_package(Threads REQUIRED)
    target_link_libraries(${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT})

    find_package(FFTW REQUIRED)
    target_include_directories(${PROJECT_NAME} PRIVATE ${FFTW_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} ${FFTW_LIBRARIES})
endif()

# install commands
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION "bin")

include(../cmake/copy_qt5_dlls_to_bin_dir.cmake)
