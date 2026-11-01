#include <doctest/doctest.h>
#include "Collision.hpp"

// ---------------------------------------------------------------------
// Overlaps() semantics (B6): two rectangles that only share an edge or a
// corner do NOT overlap. Collision.cpp compares with <=, so touching is
// treated as "not colliding". These tests pin that choice down.
// ---------------------------------------------------------------------

TEST_CASE("Overlaps: partially overlapping rectangles") {
    SDL_Rect a{ 0, 0, 50, 50 };
    SDL_Rect b{ 25, 25, 50, 50 };
    CHECK(Overlaps(a, b));
    CHECK(Overlaps(b, a)); // symmetric
}

TEST_CASE("Overlaps: one rectangle contained in another") {
    SDL_Rect outer{ 0, 0, 100, 100 };
    SDL_Rect inner{ 40, 40, 10, 10 };
    CHECK(Overlaps(outer, inner));
    CHECK(Overlaps(inner, outer));
}

TEST_CASE("Overlaps: identical rectangles") {
    SDL_Rect a{ 10, 20, 30, 40 };
    CHECK(Overlaps(a, a));
}

TEST_CASE("Overlaps: fully separated rectangles") {
    SDL_Rect a{ 0, 0, 50, 50 };
    CHECK_FALSE(Overlaps(a, SDL_Rect{ 200, 200, 50, 50 })); // apart on both axes
    CHECK_FALSE(Overlaps(a, SDL_Rect{ 200, 0, 50, 50 }));   // same rows, apart in x
    CHECK_FALSE(Overlaps(a, SDL_Rect{ 0, 200, 50, 50 }));   // same columns, apart in y
}

TEST_CASE("Overlaps: edge-touching rectangles do not overlap") {
    SDL_Rect a{ 0, 0, 50, 50 };
    CHECK_FALSE(Overlaps(a, SDL_Rect{ 50, 0, 50, 50 }));  // right edge touches left edge
    CHECK_FALSE(Overlaps(a, SDL_Rect{ 0, 50, 50, 50 }));  // bottom edge touches top edge
    CHECK_FALSE(Overlaps(a, SDL_Rect{ 50, 50, 50, 50 })); // corner touch
    CHECK_FALSE(Overlaps(SDL_Rect{ 50, 0, 50, 50 }, a));  // and the other way round
}

TEST_CASE("Overlaps: a single pixel of overlap counts") {
    SDL_Rect a{ 0, 0, 50, 50 };
    CHECK(Overlaps(a, SDL_Rect{ 49, 0, 50, 50 }));
    CHECK(Overlaps(a, SDL_Rect{ 0, 49, 50, 50 }));
}

// ---------------------------------------------------------------------
// FindSpawnPosition() (B12): bounded random placement.
// ---------------------------------------------------------------------

TEST_CASE("FindSpawnPosition: finds a position inside the window when nothing blocks") {
    auto neverBlocked = [](const SDL_Rect&) { return false; };
    for (int i = 0; i < 1000; ++i) {
        int x = -1, y = -1;
        REQUIRE(FindSpawnPosition(700, 50, 100, neverBlocked, x, y));
        CHECK(x >= 0);
        CHECK(y >= 0);
        CHECK(x + 50 <= 700);
        CHECK(y + 50 <= 700);
    }
}

TEST_CASE("FindSpawnPosition: gives up after max_attempts when everything is blocked") {
    int calls = 0;
    auto alwaysBlocked = [&](const SDL_Rect&) { ++calls; return true; };
    int x = 123, y = 456;

    CHECK_FALSE(FindSpawnPosition(700, 50, 100, alwaysBlocked, x, y));
    CHECK(calls == 100);  // exactly the cap, never more
    CHECK(x == 123);      // out params untouched on failure
    CHECK(y == 456);
}

TEST_CASE("FindSpawnPosition: zero attempts fails immediately") {
    int calls = 0;
    auto counter = [&](const SDL_Rect&) { ++calls; return false; };
    int x = 7, y = 8;

    CHECK_FALSE(FindSpawnPosition(700, 50, 0, counter, x, y));
    CHECK(calls == 0);
    CHECK(x == 7);
    CHECK(y == 8);
}

TEST_CASE("FindSpawnPosition: stops at the first free candidate") {
    int calls = 0;
    auto blockedTwice = [&](const SDL_Rect&) { return ++calls <= 2; };
    int x = -1, y = -1;

    CHECK(FindSpawnPosition(700, 50, 100, blockedTwice, x, y));
    CHECK(calls == 3); // two rejections, then the accepted one
}

TEST_CASE("FindSpawnPosition: never returns a spot inside a blocked region") {
    // Left 500 px of the window is blocked, leaving a free strip on the right.
    SDL_Rect blocked{ 0, 0, 500, 700 };
    auto isBlocked = [&](const SDL_Rect& c) { return Overlaps(c, blocked); };

    for (int i = 0; i < 200; ++i) {
        int x = -1, y = -1;
        REQUIRE(FindSpawnPosition(700, 50, 1000, isBlocked, x, y));
        CHECK_FALSE(Overlaps(SDL_Rect{ x, y, 50, 50 }, blocked));
    }
}
