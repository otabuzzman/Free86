cmake_minimum_required(VERSION 3.25)
project(free86)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/exe)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/exe)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)

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
      -Wno-unused-but-set-variable
# careful with these...
      -Wno-implicit-int-conversion
      -Wno-shadow
      -Wno-shift-bool
      -Wno-sign-compare
      -Wno-sign-conversion
    )
    add_compile_definitions(
      _CRT_SECURE_NO_WARNINGS # disable deprecation of `gmtime', `fopen'
    )
elseif(LINUX)
    add_compile_options(
      -Wall
      -Wno-sign-compare
      -Wno-unused-variable
      -Wno-unused-but-set-variable
    )
elseif(CYGWIN)
    add_compile_options(
      -Wall
      -Wno-sign-compare
      -Wno-unused-variable
      -Wno-unused-but-set-variable
    )
elseif(WIN32)
    add_compile_options(
      /W4
      /wd4018 # signed/unsigned mismatch
      /wd4100 # unreferenced parameter
      /wd4189 # local variable is initialized but not referenced
      /wd4244 # possible loss of data uint64_t to uint32_t
      /wd4267 # possible loss of data size_t  to uint32_t
      /wd4389 # signed/unsigned mismatch
      /wd4458 # declaration of 'identifier' hides class member
      /wd4996 # function or variable may be unsafe
    )
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
else()
    message("System not supported.")
endif()

file(GLOB srclib "src/*.h" "src/*.cpp")
file(GLOB srcexe "src/exe/ringbuffer.h" "src/exe/main.cpp" "src/exe/PC.h" "src/exe/PC.cpp")

add_library(free86 SHARED ${srclib})

add_executable(linos ${srcexe})
target_link_libraries(linos free86)

add_executable(test386 ${srcexe} src/exe/Test386.h)
target_compile_definitions(test386 PRIVATE TEST386)
target_link_libraries(test386 free86)

set_target_properties(linos test386 PROPERTIES LINKER_LANGUAGE CXX)

option(NO_SDL "Disable SDL support" ON)

if(NO_SDL)
    add_compile_definitions(NO_SDL)
    message(STATUS "SDL support disabled.")
else()
    find_package(OpenGL)
    target_link_libraries(linos ${OPENGL_LIBRARIES} SDL2_image SDL2_ttf SDL2 SDL2main)
endif()
