cmake_minimum_required(VERSION 3.24)
project(Rasterizer C CXX)

set(CMAKE_CXX_STANDARD 17)

include(FetchContent)

set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)

FetchContent_Declare(
        SDL3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG release-3.2.24
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(SDL3)

add_executable(Rasterizer
        main.cpp
)

target_link_libraries(Rasterizer PRIVATE
        SDL3::SDL3
)
