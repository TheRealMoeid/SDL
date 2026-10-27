cmake_minimum_required(VERSION 3.16)
project(SquareBaby LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(SquareBaby
    game.cpp
    SDLHelpers.cpp
    GameLogic.cpp
    Player.cpp
    Enemy.cpp
    Health.cpp
    Fire.cpp
    Collision.cpp
    Renderer.cpp
)

# SDL2 ships a proper CMake config on most package managers (vcpkg, apt,
# Homebrew); SDL2_image/SDL2_ttf more reliably ship only a pkg-config file,
# so pkg-config is used for all three to keep one consistent path.
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2       REQUIRED IMPORTED_TARGET sdl2)
pkg_check_modules(SDL2_IMAGE REQUIRED IMPORTED_TARGET SDL2_image)
pkg_check_modules(SDL2_TTF   REQUIRED IMPORTED_TARGET SDL2_ttf)

target_link_libraries(SquareBaby PRIVATE
    PkgConfig::SDL2
    PkgConfig::SDL2_IMAGE
    PkgConfig::SDL2_TTF
)

# Assets and consola.ttf are loaded via relative paths at runtime (see
# CLAUDE.md section 10), so they need to sit next to the built executable.
# Adjust ASSET_DIR once assets move into assets/ (repo polish, pending).
set(ASSET_DIR ${CMAKE_SOURCE_DIR})
add_custom_command(TARGET SquareBaby POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets
            $<TARGET_FILE_DIR:SquareBaby>/assets
    COMMENT "Copying game assets next to the built executable"
)