#ifndef CHARACTER_H
#define CHARACTER_H

// ---------------------------------------------------------------------------
// Character.h
//
// The part that EVERY player and EVERY enemy has in common: a lane, a
// horizontal position, health, an attack cooldown and the feedback values the
// frontend needs (healthRatio for the health bar, hitFlash for the red tint /
// scale-down described in sections 3.2 / 3.3).
//
// `attacking` is what the description calls "the characters immediately start
// attacking as soon as they are placed / appear, even if there is nothing in
// front of them" (page 4) - so it is true on the character's own rhythm and is
// NOT the same thing as `state`, which describes how the body moves.
//
// IMPORTANT RULES ENFORCED HERE:
//   * A character's lane NEVER changes after it exists (spec: players cannot
//     switch lanes, enemies must stay on their spawning lane). setLane() is
//     private on purpose - movement only ever integrates x.
//   * healthRatio() and hitFlash are pure DATA. Rendering them (health bar,
//     red shading, shrinking) is the frontend's job.
// ---------------------------------------------------------------------------

#include "Core/Types.h"

struct CharacterBase
{
    int      id;
    int      lane;         // 0 = bottom lane ... LANE_COUNT-1 = top lane
    float    x;            // horizontal position inside the lane
    int      hp;
    int      maxHp;
    float    attackCooldown;   // seconds until the next attack is allowed
    float    animTime;         // frontend: drives walk/attack animation
    float    hitFlash;         // frontend: 0..1, strength of the red damage tint
    bool     attacking;        // frontend: the weapon action is playing now
    CharState state;

    CharacterBase()
        : id(0), lane(0), x(0.0f), hp(1), maxHp(1),
          attackCooldown(0.0f), animTime(0.0f), hitFlash(0.0f),
          attacking(false), state(CharState::Idle)
    {
    }

    bool alive() const { return hp > 0 && state != CharState::Dead; }

    // 1.0 = untouched, 0.0 = dead - what the health bar displays
    float healthRatio() const
    {
        return (maxHp > 0) ? (static_cast<float>(hp) / static_cast<float>(maxHp)) : 0.0f;
    }

    // Applied to both sides. The red tint / scale-down grows as hp falls
    // (hitFlash additionally flashes right when a hit lands).
    void receiveDamage(int amount)
    {
        if (!alive()) return;
        hp -= amount;
        if (hp < 0) hp = 0;
        hitFlash = HIT_FLASH_TIME;
        if (hp == 0) {
            state = CharState::Dead;
            attacking = false;      // a corpse does not swing any more
        }
    }

    // shared per-frame bookkeeping: cooldowns, flash decay, animation clock
    void tick(float dt)
    {
        if (attackCooldown > 0.0f) attackCooldown -= dt;
        if (hitFlash > 0.0f) {
            hitFlash -= dt;
            if (hitFlash < 0.0f) hitFlash = 0.0f;
        }
        if (alive()) animTime += dt;
    }
};

#endif // CHARACTER_H
