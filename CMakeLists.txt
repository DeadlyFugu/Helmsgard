cmake_minimum_required(VERSION 3.25)
project(Helmsgard C)

set(CMAKE_C_STANDARD 11)

find_package(SDL2 REQUIRED CONFIG REQUIRED COMPONENTS SDL2)
find_package(SDL2_ttf REQUIRED CONFIG)
find_package(SDL2_mixer REQUIRED CONFIG)

set(HELMSGARD_SOURCES
        src/archive.c
        src/audio.c
        src/bfont.c
        src/command.c
        src/common.h
        src/config.c
        src/console.c
        src/dialog.c
        src/draw.c
        src/editor.c
        src/entity.c
        src/field.c
        src/game.c
        src/graphics.c
        src/ini.c
        src/input.c
        src/item.c
        src/loading.c
        src/main.c
        src/menu.c
        src/navgrid.c
        src/particles.c
        src/quest.c
        src/recipe.c
        src/region.c
        src/title.c
        src/util.c
        src/villager.c
        src/zz_sdlfc.c
        src/zz_sdlfc.h
        src/zz_tiled.h
        )

add_executable(Helmsgard WIN32 ${HELMSGARD_SOURCES})

if(TARGET SDL2::SDL2main)
    target_link_libraries(Helmsgard PRIVATE SDL2::SDL2main)
endif()
target_link_libraries(Helmsgard PRIVATE
        SDL2::SDL2
        SDL2_ttf::SDL2_ttf
        SDL2_mixer::SDL2_mixer)
