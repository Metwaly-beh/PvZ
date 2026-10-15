#ifndef COLLECTIBLES_H
#define COLLECTIBLES_H

// ---------------------------------------------------------------------------
// Collectibles.h  (sections 3.3 / 3.2)
//
//   "Defeated enemies drop collectibles."
//   "The collectibles are collected when the player collides with them."
//
// So a collectible is born where an enemy died, floats there (bobTime is only
// an animation clock for the frontend), and disappears - adding its value to
// the player's coins - as soon as any player overlaps it.
// ---------------------------------------------------------------------------

#include <vector>

#include "Core/Types.h"
#include "Player.h"

struct Collectible
{
    int   id;
    int   lane;
    float x;
    float y;
    int   value;      // coins granted on pickup
    float bobTime;    // frontend: gentle up/down animation clock
    bool  active;

    Collectible()
        : id(0), lane(0), x(0.0f), y(0.0f), value(0),
          bobTime(0.0f), active(true)
    {
    }
};

class Collectibles
{
public:
    void clear();

    // spawn a drop where an enemy was defeated
    void drop(int id, int lane, float x, int value);

    // advance the animation clocks
    void update(float dt);

    // returns the TOTAL coins picked up this frame; overlapping items are
    // marked inactive (removed by compact()).
    int  collectOverlapping(const std::vector<Player>& players);

    void compact();                                     // drop inactive items
    bool empty() const;

    const std::vector<Collectible>& items() const { return m_items; }

private:
    std::vector<Collectible> m_items;
};

#endif // COLLECTIBLES_H
