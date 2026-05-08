cmake_minimum_required(VERSION 3.25)
project(free86)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/exe)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(APPLE)
    add_compile_options(
      -Weverything
      -Wno-c++98-compat
      -Wno-comma
      -Wno-conditional-uninitialized
      -Wno-format
      -Wno-missing-noreturn
      -Wno-missing-prototypes
      -Wno-missing-variable-declarations
      -Wno-old-style-cast
      -Wno-padded
      -Wno-poison-system-directories
      -Wno-reserved-identifier
      -Wno-reserved-macro-identifier
      -Wno-shadow-field-in-constructor
      -Wno-switch-default
      -Wno-unused-parameter
      -Wno-unused-private-field
      -Wno-unused-variable
# careful with these...
      -Wno-implicit-int-conversion
      -Wno-shadow
      -Wno-sign-compare
#      -Wno-sign-conversion
    )
    add_compile_definitions(
      _CRT_SECURE_NO_WARNINGS
    )
elseif(LINUX)
    add_compile_options(
      -Wall
    )
elseif(CYGWIN)
    add_compile_options(
      -Wall
    )
elseif(WIN32)
    add_compile_options(
      /W4
    )
else()
    message("System not supported.")
endif()

file(GLOB src "src/x86/*.h" "src/x86/*.cpp" "src/*.h" "src/*.cpp")

add_executable(linos ${src})
set_target_properties(linos PROPERTIES CXX_STANDARD 20)

add_executable(test386 ${src})
set_target_properties(test386 PROPERTIES CXX_STANDARD 20)
target_compile_definitions(test386 PUBLIC TEST386)

option(NO_SDL "Disable SDL support" ON)

if(NO_SDL)
    add_compile_definitions(NO_SDL)
    message(STATUS "SDL support disabled.")
else()
    find_package(OpenGL)
    target_link_libraries(${PROJECT_NAME} ${OPENGL_LIBRARIES} SDL2_image SDL2_ttf SDL2 SDL2main)
endif()
