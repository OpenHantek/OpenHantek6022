# openhantek/CMakeLists.txt

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

if(WIN32)
    file(GLOB_RECURSE RC "res/*.rc")
endif()

add_custom_target(format SOURCES ".clang-format"
    COMMAND "clang-format" "-style=file" "-i" "-sort-includes" ${SRC} ${HEADERS})

# here the translation magic happens
include(translations/Translations.cmake)

add_definitions(-DVERSION="${CPACK_PACKAGE_VERSION}")
message( STATUS "VERSION: ${CPACK_PACKAGE_VERSION}" )

set(EXECTYPE "")
if(WIN32 AND MINGW)
    set(EXECTYPE WIN32)
endif()

# Enable MacOSX bundle magic in the next line
option(BUILD_MACOSX_BUNDLE "Build MacOS app bundle" ON)
#
if( APPLE AND BUILD_MACOSX_BUNDLE )
    set( EXECTYPE MACOSX_BUNDLE )
    set( MACOSX_BUNDLE_BUNDLE_NAME "${PROJECT_NAME}" )
    set( MACOSX_BUNDLE_INFO_STRING "Digital oscilloscope software for Hantek DSO6022" )
    set( MACOSX_BUNDLE_GUI_IDENTIFIER "${PROJECT_NAME}" )
    set( MACOSX_BUNDLE_SHORT_VERSION_STRING ${VCS_REVISION} )
    set( MACOSX_BUNDLE_BUNDLE_VERSION ${CPACK_PACKAGE_VERSION} )
    string(TIMESTAMP THISYEAR "%Y")
    set( MACOSX_BUNDLE_COPYRIGHT "(C) 2012-${THISYEAR} OpenHantek community" )
    set( MACOSX_BUNDLE_ICON_FILE openhantek.icns )
    set( ICONS ${CMAKE_CURRENT_SOURCE_DIR}/res/images/openhantek.icns )
    set_source_files_properties( ${ICONS} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources" )
endif()

# make executable
add_executable(${PROJECT_NAME} ${EXECTYPE} ${SRC} ${HEADERS} ${UI}
${QRC} ${RC} ${TRANSLATION_BIN_FILES} ${TRANSLATION_QRC} ${ICONS})
target_link_libraries(${PROJECT_NAME} Qt5::Widgets Qt5::PrintSupport Qt5::OpenGL ${OPENGL_LIBRARIES} )
target_compile_features(${PROJECT_NAME} PRIVATE cxx_range_for)
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE "/W4" "/wd4251" "/wd4127" "/wd4275" "/wd4200" "/nologo" "/J" "/Zi")
    target_compile_options(${PROJECT_NAME} PRIVATE "$<$<CONFIG:DEBUG>:/MDd>")
else()
    if( APPLE AND BUILD_MACOSX_BUNDLE )
        # Use own template that defines NSPrincipalClass=NSApplication & NSHighResolutionCapable=True
        set_target_properties( ${PROJECT_NAME} PROPERTIES
            MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_LIST_DIR}/../cmake/OpenHantekBundleInfo.plist.in
        )
    endif()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wno-long-long -pedantic)
    target_compile_options(${PROJECT_NAME} PRIVATE "$<$<CONFIG:DEBUG>:-DDEBUG>")
    target_compile_options(${PROJECT_NAME} PRIVATE "$<$<CONFIG:DEBUG>:-O0>")
    target_compile_options(${PROJECT_NAME} PRIVATE "$<$<CONFIG:RELEASE>:-fno-rtti>")
endif()

include(../cmake/fftw_on_windows.cmake)
include(../cmake/libusb_on_windows.cmake)

if(NOT WIN32)
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
if (APPLE AND BUILD_MACOSX_BUNDLE)
    #set_target_properties(${PROJECT_NAME} PROPERTIES
    #    MACOSX_BUNDLE_NAME "${PROJECT_NAME}"
    #    MACOSX_BUNDLE_GUI_IDENTIFIER "${PROJECT_NAME}"
    #)
    # no install, it's already in openhantek/OpenHantek.app/...
    # install(TARGETS ${PROJECT_NAME} BUNDLE DESTINATION ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.app/Contents/MacOS/${PROJECT_NAME})
    # install(TARGETS ${PROJECT_NAME} BUNDLE DESTINATION ${CMAKE_BINARY_DIR}) # -> build
else()
    install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION "bin")
endif()
include(../cmake/copy_qt5_dlls_to_bin_dir.cmake)

