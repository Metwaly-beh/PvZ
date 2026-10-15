#include "Collectibles.h"

#include <cmath>

namespace {
float centerYOfLane(int lane)
{
    return static_cast<float>(lane) * LANE_HEIGHT + 0.5f * LANE_HEIGHT;
}
}

void Collectibles::clear()
{
    m_items.clear();
}

void Collectibles::drop(int id, int lane, float x, int value)
{
    Collectible c;
    c.id = id;
    c.lane = lane;
    c.x = x;
    c.y = centerYOfLane(lane);
    c.value = value;
    c.bobTime = 0.0f;
    c.active = true;
    m_items.push_back(c);
}

void Collectibles::update(float dt)
{
    for (size_t i = 0; i < m_items.size(); ++i) {
        if (m_items[i].active) m_items[i].bobTime += dt;
    }
}

int Collectibles::collectOverlapping(const std::vector<Player>& players)
{
    int gathered = 0;

    for (size_t i = 0; i < m_items.size(); ++i) {
        Collectible& c = m_items[i];
        if (!c.active) continue;

        for (size_t j = 0; j < players.size(); ++j) {
            const Player& p = players[j];
            if (!p.alive()) continue;
            if (p.lane != c.lane) continue;                 // pickups are per lane

            if (std::fabs(p.x - c.x) <= PICKUP_RADIUS) {
                gathered += c.value;
                c.active = false;
                break;
            }
        }
    }
    return gathered;
}

void Collectibles::compact()
{
    std::vector<Collectible> kept;
    kept.reserve(m_items.size());
    for (size_t i = 0; i < m_items.size(); ++i)
        if (m_items[i].active) kept.push_back(m_items[i]);
    m_items.swap(kept);
}

bool Collectibles::empty() const
{
    return m_items.empty();
}
