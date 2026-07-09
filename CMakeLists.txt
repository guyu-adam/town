cmake_minimum_required(VERSION 3.20)

project(TownMarketSimulator VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(TownMarket
    src/main.cpp
    src/economy.cpp
)

if (MSVC)
    target_compile_options(TownMarket PRIVATE /W4 /permissive- /Od /Ob0 /errorReport:none)
else()
    target_compile_options(TownMarket PRIVATE -Wall -Wextra -Wpedantic)
endif()
