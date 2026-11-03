#include "CharacterSprites.h"

// ---------------------------------------------------------------------------
// CharacterSprites.cpp
//
// Defines the animation frames and sheet layouts for each character.
// Based on verified Elthen and Delta asset packs.
// ---------------------------------------------------------------------------

// Melee Player: Adventurer (32x32)
const CharacterVisuals PLAYER_MELEE = {
    { "adventurer", 32, 32, 8, 4 },
    SpriteAnim(0, 4, 0.15f, true),  // Idle
    SpriteAnim(4, 6, 0.12f, true),  // Walk
    SpriteAnim(10, 4, 0.10f, false), // Attack
    SpriteAnim(14, 6, 0.15f, false) // Death
};

// Ranged Player: Female Adventurer (32x32)
const CharacterVisuals PLAYER_RANGED = {
    { "female_adventurer", 32, 32, 8, 4 },
    SpriteAnim(0, 4, 0.15f, true),
    SpriteAnim(4, 6, 0.12f, true),
    SpriteAnim(10, 4, 0.10f, false),
    SpriteAnim(14, 6, 0.15f, false)
};

// Defensive Player: Royal Knight (128x64)
const CharacterVisuals PLAYER_TANK = {
    { "royal_knight", 128, 64, 4, 6 },
    SpriteAnim(0, 4, 0.20f, true),
    SpriteAnim(4, 6, 0.15f, true),
    SpriteAnim(10, 4, 0.12f, false),
    SpriteAnim(14, 4, 0.20f, false)
};

// Enemy Tank: Mummy Brute (96x64)
const CharacterVisuals ENEMY_TANK = {
    { "mummy_brute", 96, 64, 5, 5 },
    SpriteAnim(0, 4, 0.20f, true),
    SpriteAnim(4, 6, 0.15f, true),
    SpriteAnim(10, 5, 0.12f, false),
    SpriteAnim(15, 4, 0.20f, false)
};

// Enemy Ranged: Anubis (64x64)
const CharacterVisuals ENEMY_RANGED = {
    { "anubis", 64, 64, 6, 6 },
    SpriteAnim(0, 4, 0.15f, true),
    SpriteAnim(4, 6, 0.12f, true),
    SpriteAnim(10, 4, 0.10f, false),
    SpriteAnim(14, 4, 0.15f, false)
};

// Enemy Swarm: Scarab (32x32)
const CharacterVisuals ENEMY_SWARM = {
    { "scarab", 32, 32, 6, 4 },
    SpriteAnim(0, 4, 0.10f, true),
    SpriteAnim(4, 6, 0.10f, true),
    SpriteAnim(10, 2, 0.08f, false),
    SpriteAnim(12, 4, 0.12f, false)
};
