#pragma once

// Tunable gameplay constants (B2, B7). Kept together so a balance change
// touches one file instead of being scattered across Player/Enemy/GameLogic.
constexpr double HEALTH_GROWTH = 40.0; // px gained per health item eaten
constexpr double FIRE_PENALTY = 40.0; // px lost per fire item touched
constexpr double PLAYER_SPEED = 180.0; // px/sec, player speed
constexpr double ENEMY_SPEED = 60.0;  // px/sec, matches old 1px/frame @60fps
constexpr double ENEMY_DIRECTION_INTERVAL = 70.0 / 60.0; // seconds, matches old 70-frame timer @60fps
#define ASSET_IMAGES_DIR "assets/images/"
#define ASSET_FONTS_DIR  "assets/fonts/"