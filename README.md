# Helmsgard

*A small RPG I made for UNSW O-Week 2021*

This is a small demo of what was originally going to be a larger RPG game. It's written in C11 and uses SDL2 for rendering. Most of the game's data is specified by INI files, making it easy to edit. The map was made with Tiled.

## Building

This project uses CMake. The following dependencies are required:

- SDL2
- SDL2_ttf
- SDL2_mixer

Make sure CMake can find the dependencies. You can then build it with something like:

```shell
mkdir build
cd build
cmake ..
make
```

Or use the CMake GUI if that's more your style.

## License

All source code is provided under the zlib license,
except `zz_sdlfc.{c,h}` which is under the MIT license.

The following files may be redistributed unmodified as part of the complete game:
- `images.arc`
- `sounds.arc`
- `explore.ogg`
- `menu.ogg`

The files in `assets/images` are available under public domain.

## Credit

Parts of the following assets were used under license:
- [RPG Dungeon Tileset by Pita](https://pita.itch.io/rpg-dungeon-tileset)
- [RPG asset pack by Moose Stache](https://moose-stache.itch.io/rpg-asset-pack)
- [Ancient Game by Epic Stock Media](https://www.gamedevmarket.net/asset/ancient-game)
- [RPG Music Pack: The Complete Collection by Owl Theory Music](https://www.gamedevmarket.net/asset/rpg-music-pack-the-complete-collection)
