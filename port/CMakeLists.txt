cmake_minimum_required(VERSION 3.25)
project(GaleriansPort VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL2 CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(OpenAL CONFIG REQUIRED)

add_executable(galerians
    src/main.cpp
    src/hal/renderer.cpp
    src/hal/audio.cpp
    src/hal/input.cpp
    src/hal/gte.cpp
    src/engine/scheduler.cpp
    src/engine/game_loop.cpp
    src/engine/camera.cpp
    src/assets/cdb_loader.cpp
    src/assets/tim_loader.cpp
)

target_include_directories(galerians PRIVATE
    include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(galerians PRIVATE
    SDL2::SDL2
    SDL2::SDL2main
    OpenGL::GL
    glad::glad
    glm::glm
    OpenAL::OpenAL
)
