cmake_minimum_required(VERSION 3.28)
project(PRIM_sim LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL2 CONFIG REQUIRED)

set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/external/imgui)

add_executable(PRIM_sim
        src/main.cpp
        src/alerts.cpp
        src/prim_core.cpp
        src/ui_panels.cpp

        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_demo.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp

        ${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp
        ${IMGUI_DIR}/backends/imgui_impl_sdlrenderer2.cpp
        src/sim_types.h
        src/ui_panels.h
)

target_include_directories(PRIM_sim PRIVATE
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends
        ${CMAKE_SOURCE_DIR}/src
)


target_link_libraries(PRIM_sim PRIVATE SDL2::SDL2 SDL2::SDL2main)
