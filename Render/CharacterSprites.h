#ifndef RENDER_CHARACTERS_H
#define RENDER_CHARACTERS_H

// ---------------------------------------------------------------------------
// CharacterSprites.h
//
// How each of the six character types is drawn: which strip of the atlas it
// lives in (SheetInfo) and which frame range plays for each animation
// (CharArt's four SpriteAnim fields). The ranges are produced by
// Tools/gen_sprite_atlas.py, which lays every strip out as
// [idle][walk][attack][death] - keep the numbers here in step with it.
//
// The drawing itself lives in drawCharacters(), which is called by the
// SceneRenderer between the environment and the interface so characters
// always sit on the lanes but underneath the health bars.
// ---------------------------------------------------------------------------

#include "Core/Types.h"      // PlayerType, EnemyType, CharState
#include "SpriteAnim.h"
#include <string>

class Canvas;
class Game;

struct SheetInfo {
    std::string name;
    int fw, fh;      // frame size in pixels
    int cols, rows;  // frames are addressed row-major
    int ox, oy;      // pixel origin of the strip inside the atlas
};

struct CharArt {
    SheetInfo sheet;
    SpriteAnim idle, walk, attack, death;
    bool flipX;      // true = the sheet faces right and must be mirrored
};

extern const CharArt p_melee;    // PlayerType::Melee
extern const CharArt p_ranged;   // PlayerType::Ranged
extern const CharArt p_tank;     // PlayerType::Defense
extern const CharArt e_tank;     // EnemyType::HardMelee
extern const CharArt e_ranged;   // EnemyType::EasyUpper
extern const CharArt e_swarm;    // EnemyType::MediumLower

const CharArt& artFor(PlayerType type);
const CharArt& artFor(EnemyType type);

// Writes every player and enemy of `game` into `solid` as textured quads.
// `time` is the frame clock; the animation phase itself comes from each
// character's own animTime so it stays in step with the simulation.
void drawCharacters(const Game& game, Canvas& solid, float time);
#endif
