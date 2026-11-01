#include <doctest/doctest.h>
#include <cmath>
#include "Player.hpp"
#include "GameConstants.hpp"

namespace {
    // Large window and a start position far from every edge, so movement
    // tests are not affected by clamping.
    constexpr int BIG_WINDOW = 10000;

    PlayerState PlayerAt(double x, double y, double size = 50) {
        PlayerState p;
        p.x = x;
        p.y = y;
        p.size = size;
        return p;
    }
}

// ---------------------------------------------------------------------
// Player_HandleMovement (B19): direction vector is normalized.
// ---------------------------------------------------------------------

TEST_CASE("Movement: no keys means no movement") {
    PlayerState p = PlayerAt(5000, 5000);
    Events e;
    Player_HandleMovement(p, e, 1000, BIG_WINDOW);
    CHECK(p.x == doctest::Approx(5000));
    CHECK(p.y == doctest::Approx(5000));
}

TEST_CASE("Movement: a single axis moves at PLAYER_SPEED px/s") {
    SUBCASE("right") {
        PlayerState p = PlayerAt(5000, 5000);
        Events e; e.right = true;
        Player_HandleMovement(p, e, 1000, BIG_WINDOW);
        CHECK(p.x == doctest::Approx(5000 + PLAYER_SPEED));
        CHECK(p.y == doctest::Approx(5000));
    }
    SUBCASE("left") {
        PlayerState p = PlayerAt(5000, 5000);
        Events e; e.left = true;
        Player_HandleMovement(p, e, 1000, BIG_WINDOW);
        CHECK(p.x == doctest::Approx(5000 - PLAYER_SPEED));
        CHECK(p.y == doctest::Approx(5000));
    }
    SUBCASE("down") {
        PlayerState p = PlayerAt(5000, 5000);
        Events e; e.down = true;
        Player_HandleMovement(p, e, 1000, BIG_WINDOW);
        CHECK(p.x == doctest::Approx(5000));
        CHECK(p.y == doctest::Approx(5000 + PLAYER_SPEED));
    }
    SUBCASE("up") {
        PlayerState p = PlayerAt(5000, 5000);
        Events e; e.up = true;
        Player_HandleMovement(p, e, 1000, BIG_WINDOW);
        CHECK(p.x == doctest::Approx(5000));
        CHECK(p.y == doctest::Approx(5000 - PLAYER_SPEED));
    }
}

TEST_CASE("Movement: diagonal speed equals axis speed") {
    PlayerState p = PlayerAt(5000, 5000);
    Events e; e.right = true; e.down = true;
    Player_HandleMovement(p, e, 1000, BIG_WINDOW);

    double dx = p.x - 5000;
    double dy = p.y - 5000;
    CHECK(dx > 0);
    CHECK(dy > 0);
    CHECK(dx == doctest::Approx(dy));
    CHECK(std::sqrt(dx * dx + dy * dy) == doctest::Approx(PLAYER_SPEED));
}

TEST_CASE("Movement: opposite keys cancel exactly") {
    SUBCASE("up + down") {
        PlayerState p = PlayerAt(5000, 5000);
        Events e; e.up = true; e.down = true;
        Player_HandleMovement(p, e, 1000, BIG_WINDOW);
        CHECK(p.x == doctest::Approx(5000));
        CHECK(p.y == doctest::Approx(5000));
    }
    SUBCASE("left + right") {
        PlayerState p = PlayerAt(5000, 5000);
        Events e; e.left = true; e.right = true;
        Player_HandleMovement(p, e, 1000, BIG_WINDOW);
        CHECK(p.x == doctest::Approx(5000));
        CHECK(p.y == doctest::Approx(5000));
    }
    SUBCASE("all four keys") {
        PlayerState p = PlayerAt(5000, 5000);
        Events e; e.up = true; e.down = true; e.left = true; e.right = true;
        Player_HandleMovement(p, e, 1000, BIG_WINDOW);
        CHECK(p.x == doctest::Approx(5000));
        CHECK(p.y == doctest::Approx(5000));
    }
    SUBCASE("up + down + right still moves right at full speed") {
        PlayerState p = PlayerAt(5000, 5000);
        Events e; e.up = true; e.down = true; e.right = true;
        Player_HandleMovement(p, e, 1000, BIG_WINDOW);
        CHECK(p.x == doctest::Approx(5000 + PLAYER_SPEED));
        CHECK(p.y == doctest::Approx(5000));
    }
}

TEST_CASE("Movement: distance scales with elapsed time") {
    PlayerState p = PlayerAt(5000, 5000);
    Events e; e.right = true;
    Player_HandleMovement(p, e, 500, BIG_WINDOW);
    CHECK(p.x == doctest::Approx(5000 + PLAYER_SPEED * 0.5));

    Player_HandleMovement(p, e, 0, BIG_WINDOW); // zero delta: no change
    CHECK(p.x == doctest::Approx(5000 + PLAYER_SPEED * 0.5));
}

// ---------------------------------------------------------------------
// Player_ClampToWindow (B8)
// ---------------------------------------------------------------------

TEST_CASE("Clamp: position is pulled back inside the window") {
    SUBCASE("past the left/top edges") {
        PlayerState p = PlayerAt(-10, -20);
        Player_ClampToWindow(p, 700);
        CHECK(p.x == doctest::Approx(0));
        CHECK(p.y == doctest::Approx(0));
    }
    SUBCASE("past the right/bottom edges") {
        PlayerState p = PlayerAt(900, 800);
        Player_ClampToWindow(p, 700);
        CHECK(p.x == doctest::Approx(650)); // window - size
        CHECK(p.y == doctest::Approx(650));
    }
    SUBCASE("a bigger player has a smaller allowed range") {
        PlayerState p = PlayerAt(900, 900, 200);
        Player_ClampToWindow(p, 700);
        CHECK(p.x == doctest::Approx(500));
        CHECK(p.y == doctest::Approx(500));
    }
    SUBCASE("a position already inside is left alone") {
        PlayerState p = PlayerAt(100, 200);
        Player_ClampToWindow(p, 700);
        CHECK(p.x == doctest::Approx(100));
        CHECK(p.y == doctest::Approx(200));
    }
}

TEST_CASE("Movement: the player cannot walk out of the window") {
    PlayerState p = PlayerAt(0, 0);
    Events e; e.left = true; e.up = true;
    Player_HandleMovement(p, e, 5000, 700);
    CHECK(p.x == doctest::Approx(0));
    CHECK(p.y == doctest::Approx(0));

    Events e2; e2.right = true; e2.down = true;
    Player_HandleMovement(p, e2, 60000, 700); // a huge frame: still clamped
    CHECK(p.x == doctest::Approx(650));
    CHECK(p.y == doctest::Approx(650));
}

// ---------------------------------------------------------------------
// Player_Shrink
// ---------------------------------------------------------------------

TEST_CASE("Shrink: size decreases over time and not at all for a zero delta") {
    PlayerState p = PlayerAt(0, 0, 50);

    Player_Shrink(p, 0);
    CHECK(p.size == doctest::Approx(50));

    Player_Shrink(p, 1000);
    CHECK(p.size < 50);
    CHECK(p.size > 0);
}

TEST_CASE("Shrink: a bigger player loses more per second") {
    PlayerState small = PlayerAt(0, 0, 50);
    PlayerState big = PlayerAt(0, 0, 400);
    Player_Shrink(small, 1000);
    Player_Shrink(big, 1000);
    CHECK((400 - big.size) > (50 - small.size));
}

// ---------------------------------------------------------------------
// Player_UpdateSprite: strictly greater than 100 switches to the big sprite.
// ---------------------------------------------------------------------

TEST_CASE("Sprite: big sprite only above 100 px") {
    SDL_Texture* small_tex = reinterpret_cast<SDL_Texture*>(0x1000);
    SDL_Texture* big_tex = reinterpret_cast<SDL_Texture*>(0x2000);

    PlayerState p = PlayerAt(0, 0, 50);
    p.small_tex = small_tex;
    p.big_tex = big_tex;

    p.size = 50;
    Player_UpdateSprite(p);
    CHECK(p.current_tex == small_tex);

    p.size = 100; // boundary: not strictly greater
    Player_UpdateSprite(p);
    CHECK(p.current_tex == small_tex);

    p.size = 101;
    Player_UpdateSprite(p);
    CHECK(p.current_tex == big_tex);
}
