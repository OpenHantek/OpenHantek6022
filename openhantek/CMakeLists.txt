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

add_definitions(-DVERSION="${VERSION}")

add_subdirectory(translations)
add_subdirectory(res)

# make executable
add_executable(${PROJECT_NAME} ${SRC} ${HEADERS} ${QRC} "${CMAKE_BINARY_DIR}/translations.qrc")
target_link_libraries(${PROJECT_NAME} Qt5::Widgets Qt5::PrintSupport Qt5::OpenGL ${OPENGL_LIBRARIES} )
#target_compile_features(${PROJECT_NAME} PRIVATE cxx_range_for)

# install commands
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ".")

include(../cmake/copy_qt5_dlls_to_bin_dir.cmake)