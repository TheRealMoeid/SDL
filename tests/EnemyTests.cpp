#include <doctest/doctest.h>
#include "Enemy.hpp"
#include "Health.hpp"
#include "Fire.hpp"
#include "Collision.hpp"
#include "GameConstants.hpp"

namespace {
    constexpr int WINDOW = 700;
    constexpr Uint32 FRAME_MS = 16;

    SDL_Rect EnemyRect(const EnemyState& e) { return SDL_Rect{ (int)e.x, (int)e.y, 50, 50 }; }
}

TEST_CASE("Enemy: wanders and stays inside the window") {
    EnemyState enemy; // starts at (100, 100)
    HealthState health;
    FireState fire;
    bool moved = false;

    for (int frame = 0; frame < 20000; ++frame) {
        double bx = enemy.x, by = enemy.y;
        Enemy_Update(enemy, health, fire, FRAME_MS, WINDOW);
        if (enemy.x != bx || enemy.y != by) moved = true;

        REQUIRE(enemy.x >= 0);
        REQUIRE(enemy.x <= WINDOW - 50);
        REQUIRE(enemy.y >= 0);
        REQUIRE(enemy.y <= WINDOW - 50);
    }
    CHECK(moved);
}

TEST_CASE("Enemy: never walks into the fire or the health item (B18)") {
    EnemyState enemy;
    HealthState health;
    health.x = 400; health.y = 150; health.size = 50;
    FireState fire;
    fire.x = 250; fire.y = 300; fire.size = 50; fire.visible = true;
    SDL_Rect healthRect{ health.x, health.y, 50, 50 };
    SDL_Rect fireRect{ fire.x, fire.y, 50, 50 };

    for (int frame = 0; frame < 20000; ++frame) {
        Enemy_Update(enemy, health, fire, FRAME_MS, WINDOW);
        REQUIRE_FALSE(Overlaps(EnemyRect(enemy), healthRect));
        REQUIRE_FALSE(Overlaps(EnemyRect(enemy), fireRect));
    }
}

TEST_CASE("Enemy: keeps its direction while that direction stays open") {
    EnemyState enemy;
    enemy.x = 300; enemy.y = 300;
    enemy.direction = 3; // right
    enemy.direction_timer = 0.0;
    HealthState health;
    FireState fire;

    for (int i = 0; i < 10; ++i) { // 160 ms, far below the re-pick interval
        double before = enemy.x;
        Enemy_Update(enemy, health, fire, FRAME_MS, WINDOW);
        CHECK(enemy.direction == 3);
        CHECK(enemy.x > before);
        CHECK(enemy.y == doctest::Approx(300));
    }
}

TEST_CASE("Enemy: turns away from a wall instead of pushing into it") {
    EnemyState enemy;
    enemy.x = 649.5; enemy.y = 300; // one step from the right wall
    enemy.direction = 3;            // heading right
    HealthState health;
    FireState fire;

    Enemy_Update(enemy, health, fire, FRAME_MS, WINDOW);

    CHECK(enemy.direction != 3);
    CHECK(enemy.direction != 0);
    CHECK(enemy.x <= WINDOW - 50);
}

TEST_CASE("Enemy: freezes when every direction is blocked and resumes once free (B25)") {
    EnemyState enemy;
    enemy.x = 300; enemy.y = 300;
    HealthState health;
    FireState fire; // fire revealed right on top of the enemy
    fire.x = 300; fire.y = 300; fire.size = 50; fire.visible = true;

    Enemy_Update(enemy, health, fire, 100, WINDOW);
    CHECK(enemy.direction == 0);
    CHECK(enemy.x == doctest::Approx(300));
    CHECK(enemy.y == doctest::Approx(300));

    for (int i = 0; i < 5; ++i) Enemy_Update(enemy, health, fire, 100, WINDOW);
    CHECK(enemy.x == doctest::Approx(300)); // still frozen on later frames
    CHECK(enemy.y == doctest::Approx(300));

    fire.visible = false; // the fire expires
    Enemy_Update(enemy, health, fire, 100, WINDOW);
    CHECK(enemy.direction != 0);
    CHECK((enemy.x != doctest::Approx(300) || enemy.y != doctest::Approx(300)));
}
