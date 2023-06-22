cmake_minimum_required(VERSION 3.25)
project(Helmsgard C)

set(CMAKE_C_STANDARD 11)

find_package(SDL2 REQUIRED CONFIG COMPONENTS SDL2)
find_package(SDL2_ttf REQUIRED CONFIG)

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

add_executable(Helmsgard ${HELMSGARD_SOURCES})
#target_include_directories(Helmsgard PUBLIC ${SDL2_INCLUDE_DIRS} ${SDL2_TTF_INCLUDE_DIRS})
#target_link_libraries(Helmsgard PUBLIC ${SDL2_LIBRARIES} ${SDL2_TTF_LIBRARIES})
target_link_libraries(Helmsgard PRIVATE SDL2::SDL2 SDL2_ttf::SDL2_ttf)
