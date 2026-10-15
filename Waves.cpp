#include "Waves.h"

Waves::Waves()
    : m_waveIndex(-1), m_spawnTimer(0.0f), m_phase(WavePhase::NotStarted)
{
    // the three waves of section 3.3
    WaveSpec w1; w1.minEnemies = WAVE1_MIN_ENEMIES;
    w1.allowEasy = true; w1.allowMedium = false; w1.allowHard = false;

    WaveSpec w2; w2.minEnemies = WAVE2_MIN_ENEMIES;
    w2.allowEasy = true; w2.allowMedium = true; w2.allowHard = false;

    WaveSpec w3; w3.minEnemies = WAVE3_MIN_ENEMIES;
    w3.allowEasy = true; w3.allowMedium = true; w3.allowHard = true;

    m_specs.push_back(w1);
    m_specs.push_back(w2);
    m_specs.push_back(w3);

    reset();
}

void Waves::reset()
{
    m_queue.clear();
    m_waveIndex = -1;
    m_spawnTimer = 0.0f;
    m_phase = WavePhase::NotStarted;
}

int Waves::waveCount() const
{
    return static_cast<int>(m_specs.size());
}

const WaveSpec& Waves::spec(int wave) const
{
    static WaveSpec fallback;
    if (wave < 0 || wave >= static_cast<int>(m_specs.size())) return fallback;
    return m_specs[wave];
}

int Waves::remainingToSpawn() const
{
    return static_cast<int>(m_queue.size());
}

int Waves::aliveCount(const std::vector<Enemy>& enemies) const
{
    int n = 0;
    for (size_t i = 0; i < enemies.size(); ++i)
        if (enemies[i].alive()) ++n;
    return n;
}

void Waves::beginWave(int wave, Random& rng)
{
    const WaveSpec& s = spec(wave);

    // "Minimum Number of Enemies" - sometimes one or two extra for variety
    int count = s.minEnemies + rng.range(0, 1);

    std::vector<EnemyType> pool;
    std::vector<EnemyType> allowed;
    if (s.allowEasy)   allowed.push_back(EnemyType::EasyUpper);
    if (s.allowMedium) allowed.push_back(EnemyType::MediumLower);
    if (s.allowHard)   allowed.push_back(EnemyType::HardMelee);

    // "A mix of ..." - make sure every allowed type really shows up, then
    // fill the rest of the wave at random.
    for (size_t k = 0; k < allowed.size() && static_cast<int>(pool.size()) < count; ++k)
        pool.push_back(allowed[k]);

    while (static_cast<int>(pool.size()) < count && !allowed.empty()) {
        int pick = rng.index(static_cast<int>(allowed.size()));
        pool.push_back(allowed[pick]);
    }

    m_queue = pool;
    m_spawnTimer = 0.0f;      // the first enemy appears immediately: the wave
    m_phase = WavePhase::Spawning;   // BEGINS the moment it shows up
}

int Waves::pickLane(const std::vector<Enemy>& enemies, Random& rng) const
{
    // try a few times to land on a lane that nobody is walking down yet;
    // this keeps a wave from stacking up in a single lane
    const int MAX_TRIES = 4;
    for (int attempt = 0; attempt < MAX_TRIES; ++attempt) {
        int lane = rng.range(0, LANE_COUNT - 1);

        bool busy = false;
        for (size_t i = 0; i < enemies.size(); ++i) {
            if (enemies[i].alive() && enemies[i].lane == lane) { busy = true; break; }
        }
        if (!busy) return lane;
    }

    // every lane is busy (late in a wave) - any lane will do
    return rng.range(0, LANE_COUNT - 1);
}

Enemy Waves::spawnOne(Random& rng, int& nextId, const std::vector<Enemy>& enemies) const
{
    EnemyType type = m_queue.front();
    int lane = pickLane(enemies, rng);            // "in a randomly chosen lane"
    Enemy e = makeEnemy(nextId++, type, lane, ENEMY_SPAWN_X);
    return e;
}

int Waves::update(float dt, std::vector<Enemy>& enemies, int& nextId, Random& rng)
{
    // the very first call of a round kicks wave 1 off
    if (m_waveIndex == -1) {
        m_waveIndex = 0;
        beginWave(0, rng);
    }

    if (m_phase == WavePhase::Spawning) {
        if (m_queue.empty()) {
            m_phase = WavePhase::Clearing;
            return 0;
        }

        m_spawnTimer -= dt;
        if (m_spawnTimer > 0.0f) return 0;

        enemies.push_back(spawnOne(rng, nextId, enemies));
        m_queue.erase(m_queue.begin());
        m_spawnTimer = WAVE_SPAWN_INTERVAL;

        if (m_queue.empty()) {
            // the FINAL enemy of this wave just appeared -> the wave has ENDED
            m_phase = WavePhase::Clearing;
        }
        return 1;
    }

    if (m_phase == WavePhase::Clearing) {
        // "the player must defeat the remaining enemies on screen to proceed
        //  to the next wave until all waves are covered"
        if (aliveCount(enemies) == 0) {
            ++m_waveIndex;
            if (m_waveIndex < waveCount()) {
                beginWave(m_waveIndex, rng);
            }
            else {
                m_phase = WavePhase::NotStarted;   // every wave is done
            }
        }
    }

    return 0;
}

bool Waves::allCleared(const std::vector<Enemy>& enemies) const
{
    bool everyWaveSpawned = (m_waveIndex >= waveCount());
    return everyWaveSpawned && m_queue.empty() && aliveCount(enemies) == 0;
}
