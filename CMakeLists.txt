cmake_minimum_required(VERSION 3.22)
project(tf2_detour_library)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m64")

add_library(tf2_detour_library SHARED main.cpp
)

#include_directories(/usr/local/include)

find_library(FUNCHOOK_LIBRARY NAMES funchook)

if (NOT FUNCHOOK_LIBRARY)
    message(FATAL_ERROR "Funchook library not found")
endif ()

target_link_libraries(tf2_detour_library ${FUNCHOOK_LIBRARY})