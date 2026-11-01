#include <doctest/doctest.h>
#include <cstdlib>
#include "Health.hpp"
#include "Fire.hpp"
#include "Player.hpp"
#include "Enemy.hpp"
#include "Collision.hpp"
#include "GameConstants.hpp"

namespace {
    constexpr int WINDOW = 700;

    SDL_Rect PlayerRect(const PlayerState& p) { return SDL_Rect{ (int)p.x, (int)p.y, (int)p.size, (int)p.size }; }
    SDL_Rect EnemyRect(const EnemyState& e) { return SDL_Rect{ (int)e.x, (int)e.y, 50, 50 }; }
    SDL_Rect HealthRect(const HealthState& h) { return SDL_Rect{ h.x, h.y, (int)h.size, (int)h.size }; }
    SDL_Rect FireRect(const FireState& f) { return SDL_Rect{ f.x, f.y, (int)f.size, (int)f.size }; }

    // A player as large as the window: every spawn candidate overlaps it,
    // so there is no valid position anywhere.
    PlayerState WindowFillingPlayer() {
        PlayerState p;
        p.x = 0;
        p.y = 0;
        p.size = WINDOW;
        return p;
    }
}

// ---------------------------------------------------------------------
// Health_SpawnIfNeeded (B12)
// ---------------------------------------------------------------------

TEST_CASE("Health_SpawnIfNeeded: leaves an existing item alone") {
    HealthState health;
    health.x = 11;
    health.y = 22;
    health.size = 30;
    PlayerState player;
    EnemyState enemy;
    FireState fire;

    Health_SpawnIfNeeded(health, player, enemy, fire, WINDOW);

    CHECK(health.x == 11);
    CHECK(health.y == 22);
    CHECK(health.size == doctest::Approx(30));
}

TEST_CASE("Health_SpawnIfNeeded: spawns a full-size item that avoids player, enemy and visible fire") {
    PlayerState player;
    player.x = 300; player.y = 300; player.size = 80;
    EnemyState enemy;
    enemy.x = 100; enemy.y = 500;
    FireState fire;
    fire.x = 500; fire.y = 100; fire.size = 50; fire.visible = true;

    for (int i = 0; i < 500; ++i) {
        HealthState health; // size -1: needs a spawn
        Health_SpawnIfNeeded(health, player, enemy, fire, WINDOW);

        REQUIRE(health.size == doctest::Approx(ITEM_SPAWN_SIZE));
        SDL_Rect h = HealthRect(health);
        CHECK(h.x >= 0);
        CHECK(h.y >= 0);
        CHECK(h.x + h.w <= WINDOW);
        CHECK(h.y + h.h <= WINDOW);
        CHECK_FALSE(Overlaps(h, PlayerRect(player)));
        CHECK_FALSE(Overlaps(h, EnemyRect(enemy)));
        CHECK_FALSE(Overlaps(h, FireRect(fire)));
    }
}

TEST_CASE("Health_SpawnIfNeeded: a fully blocked window fails instead of looping (B12)") {
    PlayerState player = WindowFillingPlayer();
    EnemyState enemy;
    FireState fire;
    HealthState health;
    health.x = 5;
    health.y = 6;

    // Reaching the checks below at all proves the call terminated.
    Health_SpawnIfNeeded(health, player, enemy, fire, WINDOW);

    CHECK(health.size <= 0); // still "needs a spawn", retried next frame
    CHECK(health.x == 5);    // position untouched on failure
    CHECK(health.y == 6);
}

TEST_CASE("Health_SpawnIfNeeded: a hidden fire does not block spawning") {
    PlayerState player;
    player.x = 600; player.y = 600; player.size = 50;
    EnemyState enemy;
    enemy.x = 0; enemy.y = 0;
    FireState fire; // stale geometry covering the whole window, but not visible
    fire.x = 0; fire.y = 0; fire.size = WINDOW; fire.visible = false;
    HealthState health;

    Health_SpawnIfNeeded(health, player, enemy, fire, WINDOW);

    CHECK(health.size == doctest::Approx(ITEM_SPAWN_SIZE));
}

TEST_CASE("Health_Shrink: loses 10 px per second") {
    HealthState health;
    health.size = 50;

    Health_Shrink(health, 1000);
    CHECK(health.size == doctest::Approx(40));

    Health_Shrink(health, 0);
    CHECK(health.size == doctest::Approx(40));

    Health_Shrink(health, 4000);
    CHECK(health.size <= 0);
}

// ---------------------------------------------------------------------
// Fire_Reveal (B25): the position is chosen against where everything is
// right now, at the moment the fire appears.
// ---------------------------------------------------------------------

TEST_CASE("Fire_Reveal: reveals a full-size fire that avoids player, enemy and health") {
    PlayerState player;
    player.x = 300; player.y = 300; player.size = 80;
    HealthState health;
    health.x = 500; health.y = 100; health.size = 50;

    for (int i = 0; i < 500; ++i) {
        // The enemy can be anywhere when the fire appears (B25 scenario).
        EnemyState enemy;
        enemy.x = rand() % 650;
        enemy.y = rand() % 650;
        FireState fire;

        REQUIRE(Fire_Reveal(fire, player, enemy, health, WINDOW));

        CHECK(fire.visible);
        CHECK(fire.size == doctest::Approx(ITEM_SPAWN_SIZE));
        SDL_Rect f = FireRect(fire);
        CHECK(f.x >= 0);
        CHECK(f.y >= 0);
        CHECK(f.x + f.w <= WINDOW);
        CHECK(f.y + f.h <= WINDOW);
        CHECK_FALSE(Overlaps(f, EnemyRect(enemy)));
        CHECK_FALSE(Overlaps(f, PlayerRect(player)));
        CHECK_FALSE(Overlaps(f, HealthRect(health)));
    }
}

TEST_CASE("Fire_Reveal: a fully blocked window leaves the fire hidden") {
    PlayerState player = WindowFillingPlayer();
    EnemyState enemy;
    HealthState health;
    FireState fire;
    fire.x = 9;
    fire.y = 8;

    CHECK_FALSE(Fire_Reveal(fire, player, enemy, health, WINDOW));

    CHECK_FALSE(fire.visible);
    CHECK(fire.size <= 0);
    CHECK(fire.x == 9);
    CHECK(fire.y == 8);
}

// ---------------------------------------------------------------------
// Fire_Shrink
// ---------------------------------------------------------------------

TEST_CASE("Fire_Shrink: hidden fire is left alone") {
    FireState fire; // hidden, size -1
    Fire_Shrink(fire, 1000);
    CHECK_FALSE(fire.visible);
    CHECK(fire.size == doctest::Approx(-1));
}

TEST_CASE("Fire_Shrink: visible fire loses 10 px per second") {
    FireState fire;
    fire.size = 50;
    fire.visible = true;

    Fire_Shrink(fire, 1000);

    CHECK(fire.visible);
    CHECK(fire.size == doctest::Approx(40));
}

TEST_CASE("Fire_Shrink: fire disappears once it reaches zero") {
    FireState fire;
    fire.size = 5;
    fire.visible = true;

    Fire_Shrink(fire, 1000); // 5 - 10 -> gone

    CHECK_FALSE(fire.visible);
    CHECK(fire.size == doctest::Approx(-1)); // back to the "hidden" sentinel
}
