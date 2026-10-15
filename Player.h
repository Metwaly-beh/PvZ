#ifndef PLAYER_H
#define PLAYER_H

// ---------------------------------------------------------------------------
// Player.h  (section 3.2 - Player Logic)
//
//   Melee   : close range weapon, advances towards the right, jumps & ducks.
//   Ranged  : fires projectiles that disappear on collision, advances, jumps & ducks.
//   Defense : NEVER moves after being placed, cannot jump or duck, acts as a
//             blocker that absorbs enemy attacks for players behind it.
//
// Every player type deals exactly 1 point of damage per attack.
// Players may overlap / pass through each other - there is deliberately NO
// player vs player collision (section 3.8).
// ---------------------------------------------------------------------------

#include <vector>

#include "Character.h"
#include "Core/Types.h"
#include "Playfield.h"

struct Player : public CharacterBase
{
    PlayerType type;
    float      jumpTimer;    // > 0 while airborne  -> dodges LOW attacks
    float      duckTimer;    // > 0 while crouched  -> dodges HIGH attacks

    Player()
        : type(PlayerType::Melee), jumpTimer(0.0f), duckTimer(0.0f)
    {
    }

    // spec: "players may temporarily overlap or pass through one another",
    //       "the Defense Player is the only player type that does not move"
    bool canMove() const { return type != PlayerType::Defense; }

    // spec: "Defense players are stationary and are not affected by jump or duck"
    bool canChangeStance() const { return type != PlayerType::Defense; }

    bool isJumping() const { return canChangeStance() && jumpTimer > 0.0f; }
    bool isDucking() const { return canChangeStance() && duckTimer > 0.0f; }

    // section 3.8: "jumping avoids low attacks and ducking avoids high attacks"
    // Straight attacks (melee swings / straight shots) hit regardless.
    bool dodges(AttackHeight h) const
    {
        switch (h) {
        case AttackHeight::High:   return isDucking();
        case AttackHeight::Low:    return isJumping();
        case AttackHeight::Straight: break;
        }
        return false;
    }

    void tickPlayer(float dt)
    {
        tick(dt);
        if (jumpTimer > 0.0f) jumpTimer -= dt;
        if (duckTimer > 0.0f) duckTimer -= dt;
        if (jumpTimer < 0.0f) jumpTimer = 0.0f;
        if (duckTimer < 0.0f) duckTimer = 0.0f;
    }
};

// factory: fills hp / cost-relevant fields from the constants in Types.h
Player makePlayer(int id, PlayerType type, int lane, float x);

// W / Up -> every player in the SELECTED lanes jumps,
// S / Down -> every player in the SELECTED lanes ducks.
// Returns how many players were told to change stance (0 when no lane is
// selected - that is the spec's "per lane" behaviour, section 3.8).
int commandJump(std::vector<Player>& players, const Playfield& playfield);
int commandDuck(std::vector<Player>& players, const Playfield& playfield);

// walks towards the right, never past the given destination, and only when
// nothing blocked it (the caller decides whether a blocking enemy stopped it)
void advancePlayer(Player& p, float dt, float destX);

// section 3.2: "the Defense Player provides protection ... as long as the
// player remains behind the Defense Player. Once the player moves past the
// Defense Player ... the player will become vulnerable."
//
// Enemies always come from the right, so a shield must sit at an x >= the
// player's x in the same lane. Returns 0 when nothing shields that player
// (the Defense player itself is never shielded by another one).
const Player* findDefenseShield(const std::vector<Player>& players, const Player& p);

#endif // PLAYER_H
