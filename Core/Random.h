#ifndef CORE_RANDOM_H
#define CORE_RANDOM_H

// ---------------------------------------------------------------------------
// Random.h
//
// One tiny wrapper around std::mt19937 so every random decision of the game
// (which lane does the next enemy spawn in, how many enemies does a wave
// have, which enemy types a wave mixes) comes from ONE place.
//
// It can be seeded explicitly, which lets the test harness replay the exact
// same game twice and assert that the logic is deterministic.
// ---------------------------------------------------------------------------

#include <random>

class Random
{
public:
    Random() : m_engine(std::random_device{}()) {}

    void seed(unsigned int s) { m_engine.seed(s); }

    // inclusive range
    int range(int lo, int hi)
    {
        if (hi <= lo) return lo;
        std::uniform_int_distribution<int> dist(lo, hi);
        return dist(m_engine);
    }

    // picks a random element index, or -1 when the container is empty
    int index(int count)
    {
        if (count <= 0) return -1;
        return range(0, count - 1);
    }

private:
    std::mt19937 m_engine;
};

#endif // CORE_RANDOM_H
