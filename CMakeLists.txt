cmake_minimum_required(VERSION 3.20)
project(SloppyFocus LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT WIN32)
    message(FATAL_ERROR "SloppyFocus is Windows-only.")
endif()

add_executable(SloppyFocus WIN32
    src/main.cpp
    res/app.rc
)

target_include_directories(SloppyFocus PRIVATE src res)

if(MSVC)
    target_compile_options(SloppyFocus PRIVATE /W4 /permissive- /utf-8)
endif()

target_compile_definitions(SloppyFocus PRIVATE
    UNICODE _UNICODE
    WIN32_LEAN_AND_MEAN NOMINMAX
)

target_link_libraries(SloppyFocus PRIVATE user32 shell32 advapi32 comctl32)

set_target_properties(SloppyFocus PROPERTIES
    OUTPUT_NAME "SloppyFocus"
)
