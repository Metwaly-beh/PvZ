#ifndef PROJECTILE_H
#define PROJECTILE_H

// ---------------------------------------------------------------------------
// Projectile.h
//
// Everything that flies along a lane:
//
//   * players (Ranged) fire STRAIGHT shots - "fires a projectile; ... disappears
//     on collision" and it NEVER harms other players (only enemies are tested
//     against it, see Game::updateProjectiles).
//   * EasyUpper enemies fire HIGH shots   -> a crouching player dodges them.
//   * MediumLower enemies fire LOW shots  -> a jumping player dodges them.
//
// A projectile belongs to exactly one lane and moves horizontally only.
//
// It also has a RANGE: it is fired by an attacker whose weapon only reaches
// RANGED_RANGE units, so a shot that hits nobody simply expires after flying
// that far. (This is what keeps the "shoot even with no target" rule of page 4
// harmless - stray shots may not snipe a character from across the playfield.)
// ---------------------------------------------------------------------------

#include "Core/Types.h"

struct Projectile
{
    int          id;
    Faction      faction;
    int          lane;
    float        x;         // world x of the tip
    float        y;         // world y (lane center) - for the frontend
    float        vx;        // signed speed: + right, - left
    float        originX;   // where it was fired from (range is measured from here)
    float        range;     // how far it can travel before it expires
    AttackHeight height;
    int          damage;
    int          sourceId;  // who fired it (player or enemy id)
    bool         active;

    Projectile()
        : id(0), faction(Faction::Player), lane(0), x(0.0f), y(0.0f),
          vx(0.0f), originX(0.0f), range(RANGED_RANGE),
          height(AttackHeight::Straight), damage(1),
          sourceId(0), active(true)
    {
    }

    // direction of travel as -1 / +1, handy for "how far has it gone?"
    int direction() const { return (vx < 0.0f) ? -1 : 1; }

    float distanceTravelled() const
    {
        float d = x - originX;
        return (d < 0.0f) ? -d : d;
    }
};

Projectile makeProjectile(int id, Faction faction, int lane, float x,
                          float laneCenterY, float vx, AttackHeight height,
                          int damage, int sourceId);

#endif // PROJECTILE_H
