cmake_minimum_required(VERSION 3.20)

project(TownMarketSimulator VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.5
)
FetchContent_MakeAvailable(raylib)

add_executable(TownMarket
    src/main.cpp
    src/economy.cpp
    src/render_map.cpp
    src/render_ui.cpp
    src/render_charts.cpp
    src/sprites.cpp
    src/camera.cpp
    src/ui_popup.cpp
)

target_link_libraries(TownMarket PRIVATE raylib)

if (MSVC)
    target_compile_options(TownMarket PRIVATE /W4 /permissive- /Od /Ob0 /errorReport:none)
else()
    target_compile_options(TownMarket PRIVATE -Wall -Wextra -Wpedantic)
endif()

if (WIN32)
    target_link_libraries(TownMarket PRIVATE winmm)
endif()
