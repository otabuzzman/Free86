cmake_minimum_required(VERSION 3.12)
project(cpp_app)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/exe)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB sourcefiles "src/x86/*.h" "src/x86/*.cpp" "src/*.h" "src/*.cpp")

add_compile_options(
  -Weverything
  -Wno-c++98-compat
  -Wno-conditional-uninitialized
  -Wno-missing-noreturn
  -Wno-missing-prototypes
  -Wno-missing-variable-declarations
  -Wno-padded
  -Wno-poison-system-directories
  -Wno-reserved-identifier
  -Wno-reserved-macro-identifier
  -Wno-unused-private-field
)
add_compile_definitions(
  _CRT_SECURE_NO_WARNINGS
)

add_executable(${PROJECT_NAME} ${sourcefiles})

set_property(TARGET cpp_app PROPERTY CXX_STANDARD 20)

option(NO_SDL "Disable SDL support" OFF)
option(TEST386 "Run test386 suite" OFF)

if(NO_SDL)
    add_compile_definitions(NO_SDL)
    message(STATUS "SDL support disabled.")
else()
    find_package(OpenGL)
    target_link_libraries(${PROJECT_NAME} ${OPENGL_LIBRARIES} SDL2_image SDL2_ttf SDL2 SDL2main)
endif()

if(TEST386)
    add_compile_definitions(TEST386)
    message(STATUS "Run test386 suite.")
endif()
