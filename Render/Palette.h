#ifndef RENDER_PALETTE_H
#define RENDER_PALETTE_H

// ---------------------------------------------------------------------------
// Palette.h
//
// The art direction of "The Sealed Tomb" in one place: every visual file takes
// its colours from here instead of inventing its own, which is what keeps the
// scene looking like one game. Each phase of the artwork adds the entries it
// needs.
//
//   Guardians (players) : carved stone, linen, bronze, lapis and gold
//   Desecrators (enemies): rag, ash, terracotta, jade and iron
// ---------------------------------------------------------------------------

#include "Core.h"

// --- shared ----------------------------------------------------------------

const Color OUTLINE_DARK    = color(0x2A1B10);   // warm black, every shape
const Color OUTLINE_PLAYER  = color(0x3A2A16);   // guardians read warm ...
const Color OUTLINE_ENEMY   = color(0x2E2531);   // ... desecrators read cold

const Color SHADOW_WARM     = color(0x000000, 0.30f);   // contact shadow
const Color DUST            = color(0xD9BE92);

// --- guardians ------------------------------------------------------------

const Color BRONZE          = color(0xC8862F);
const Color BRONZE_DARK     = color(0x8A5A1B);
const Color BRONZE_LIGHT    = color(0xF2C46B);
const Color LINEN           = color(0xE8DCC0);
const Color LINEN_SHADE     = color(0xC6B48C);
const Color LAPIS           = color(0x2E5FA3);
const Color GOLD            = color(0xE9B949);
const Color LOTUS           = color(0xD98BA0);
const Color IVORY           = color(0xEFE7D6);
const Color EBONY           = color(0x4A3320);

const Color STONE           = color(0xCDBFA4);   // limestone, the gate & statue
const Color STONE_SHADE     = color(0x8E7F66);
const Color STONE_LIGHT     = color(0xEFE4CC);

// --- desecrators ----------------------------------------------------------

const Color RAG             = color(0x9A7B4F);
const Color RAG_DARK        = color(0x6B523A);
const Color TERRACOTTA      = color(0xB4643C);
const Color ASH             = color(0x8A8071);
const Color JADE            = color(0x5F7A3E);
const Color JADE_DARK       = color(0x3E5229);
const Color IRON            = color(0x6B6B72);
const Color IRON_DARK       = color(0x3A3A42);
const Color BONE            = color(0xDDD3BE);
const Color LEATHER         = color(0x5A4230);

// --- damage feedback (section 3.8 / 4.1) ----------------------------------

const Color HIT_RED         = color(0xC03028);
const Color HEALTH_HIGH     = color(0x5FBF5A);
const Color HEALTH_MID      = color(0xE8B33C);
const Color HEALTH_LOW      = color(0xC74A3C);
const Color HEALTH_FRAME    = color(0x241A12);

// --- environment (phase 2: the sealed tomb courtyard) ----------------------

const Color SKY_TOP         = color(0x12152E);   // indigo overhead
const Color SKY_MID         = color(0x2C2350);   // violet
const Color SKY_HORIZON     = color(0x7A4460);   // dusty rose at the horizon
const Color STAR            = color(0xEFE7D6);
const Color MOON            = color(0xF2E3B0);
const Color MOON_GLOW       = color(0xBFD0FF);
const Color DUNE_FAR        = color(0x4A4068);   // haze lifts what is far away
const Color DUNE_NEAR       = color(0x322A52);
const Color PYRAMID_FAR     = color(0x3E3660);
const Color PYRAMID_NEAR    = color(0x2A2446);

const Color WALL            = color(0x8E7F66);   // temple wall limestone
const Color WALL_LIGHT      = color(0xB6A688);
const Color WALL_DARK       = color(0x6B6049);
const Color WALL_SHADOW     = color(0x40382B);
const Color WALL_GLYPH      = color(0x544A38);

const Color SAND            = color(0xC4A87A);   // courtyard floor
const Color SAND_LIGHT      = color(0xDCC79C);
const Color SAND_DARK       = color(0x9E835C);
const Color LANE_WORN       = color(0xE0CBA1);   // the path characters walk

const Color SUN_CORE        = color(0xFFF3C8);   // the Sunwell itself
const Color SUN_EDGE        = color(0xF2B23F);
const Color FLAME           = color(0xFF9A3C);
const Color FLAME_CORE      = color(0xFFE9A8);

#endif // RENDER_PALETTE_H
