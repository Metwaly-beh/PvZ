#ifndef RENDER_CHARACTERS_H
#define RENDER_CHARACTERS_H

#include "SpriteAnim.h"
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// CharacterSprites.h
//
// Maps game roles to specific sprite sheet metadata.
// This keeps the artwork logic separate from the gameplay logic.
// ---------------------------------------------------------------------------

struct SpriteSheetInfo {
    std::string assetName;
    int frameWidth;
    int frameHeight;
    int cols;
    int rows;
};

struct CharacterVisuals {
    SpriteSheetInfo sheet;
    SpriteAnim idle;
    SpriteAnim walk;
    SpriteAnim attack;
    SpriteAnim death;
};

// The defined visual sets for our six characters.
extern const CharacterVisuals PLAYER_MELEE;
extern const CharacterVisuals PLAYER_RANGED;
extern const CharacterVisuals PLAYER_TANK;
extern const CharacterVisuals ENEMY_TANK;
extern const CharacterVisuals ENEMY_RANGED;
extern const CharacterVisuals ENEMY_SWARM;

#endif // RENDER_CHARACTERS_H
