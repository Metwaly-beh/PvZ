#ifndef RENDER_CHARACTERS_H
#define RENDER_CHARACTERS_H
#include "SpriteAnim.h"
#include <string>

struct SheetInfo {
    std::string name;
    int fw, fh, cols, rows;
};

struct CharArt {
    SheetInfo sheet;
    SpriteAnim idle, walk, attack, death;
};

extern const CharArt p_melee;
extern const CharArt p_ranged;
extern const CharArt p_tank;
extern const CharArt e_tank;
extern const CharArt e_ranged;
extern const CharArt e_swarm;
#endif
