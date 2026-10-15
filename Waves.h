#ifndef WAVES_H
#define WAVES_H

// ---------------------------------------------------------------------------
// Waves.h  (section 3.3 - enemy waves)
//
//   Wave 1 : at least 3 enemies, Easy only
//   Wave 2 : at least 5 enemies, a mix of easy and medium
//   Wave 3 : at least 10 enemies, a mix of easy, medium and hard
//
// Lifecycle exactly as described:
//   * a wave BEGINS as soon as its first enemy appears on screen,
//   * it ENDS as soon as the final enemy of that wave appears (the spawn
//     queue is empty -> phase Clearing, "no additional enemies from that
//     wave will spawn"),
//   * the player must then defeat everything that is still on screen before
//     the next wave is allowed to start.
//
// The lane of every spawn is RANDOM (section 3.1 "enemy entry"). Purely
// uniform picking, however, happily dumps a whole wave into one lane, which no
// amount of clever play can survive at the start of the game. So a spawn first
// tries to find a lane that is not already occupied, and only falls back to a
// plain random lane once every lane is busy - still random, just spread.
// ---------------------------------------------------------------------------

#include <vector>

#include "Core/Random.h"
#include "Core/Types.h"
#include "Enemy.h"

struct WaveSpec
{
    int  minEnemies;      // "Minimum Number of Enemies"
    bool allowEasy;
    bool allowMedium;
    bool allowHard;
};

class Waves
{
public:
    Waves();

    void reset();

    // called every frame while the game is Playing. Spawns enemies into the
    // given vector (at ENEMY_SPAWN_X of a random lane) and returns how many
    // spawns happened this frame (0 or 1) - the game turns that into a
    // WaveStarted event when the FIRST spawn of a wave happens.
    int  update(float dt, std::vector<Enemy>& enemies, int& nextId, Random& rng);

    // true once EVERY wave has been spawned and the screen is empty again
    bool allCleared(const std::vector<Enemy>& enemies) const;

    int         currentWave() const { return m_waveIndex; }   // 0-based
    int         waveCount() const;
    WavePhase   phase() const { return m_phase; }
    int         remainingToSpawn() const;
    int         aliveCount(const std::vector<Enemy>& enemies) const;

    const WaveSpec& spec(int wave) const;

private:
    void  beginWave(int wave, Random& rng);
    Enemy spawnOne(Random& rng, int& nextId, const std::vector<Enemy>& enemies) const;
    int   pickLane(const std::vector<Enemy>& enemies, Random& rng) const;

    std::vector<WaveSpec> m_specs;
    std::vector<EnemyType> m_queue;     // still to appear for the current wave
    int          m_waveIndex;           // -1 = nothing started yet
    float        m_spawnTimer;          // counts down to the next spawn
    WavePhase    m_phase;
};

#endif // WAVES_H
