#include "CharacterSprites.h"

const CharArt p_melee = { {"adventurer", 32, 32, 8, 4}, SpriteAnim(0, 4, 0.15f, true), SpriteAnim(4, 6, 0.12f, true), SpriteAnim(10, 4, 0.10f, false), SpriteAnim(14, 6, 0.15f, false) };
const CharArt p_ranged = { {"female_adventurer", 32, 32, 8, 4}, SpriteAnim(0, 4, 0.18f, true), SpriteAnim(4, 6, 0.14f, true), SpriteAnim(10, 3, 0.08f, false), SpriteAnim(14, 6, 0.12f, false) };
const CharArt p_tank = { {"royal_knight", 128, 64, 4, 6}, SpriteAnim(0, 4, 0.25f, true), SpriteAnim(4, 6, 0.20f, true), SpriteAnim(10, 3, 0.15f, false), SpriteAnim(14, 4, 0.25f, false) };
const CharArt e_tank = { {"mummy_brute", 96, 64, 5, 5}, SpriteAnim(0, 4, 0.22f, true), SpriteAnim(4, 6, 0.18f, true), SpriteAnim(10, 5, 0.12f, false), SpriteAnim(15, 4, 0.22f, false) };
const CharArt e_ranged = { {"anubis", 64, 64, 6, 6}, SpriteAnim(0, 4, 0.12f, true), SpriteAnim(4, 6, 0.10f, true), SpriteAnim(10, 4, 0.08f, false), SpriteAnim(14, 4, 0.12f, false) };
const CharArt e_swarm = { {"scarab", 32, 32, 6, 4}, SpriteAnim(0, 4, 0.08f, true), SpriteAnim(4, 6, 0.08f, true), SpriteAnim(10, 2, 0.05f, false), SpriteAnim(12, 4, 0.08f, false) };
