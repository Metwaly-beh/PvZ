#ifndef RENDER_ENVIRONMENT_H
#define RENDER_ENVIRONMENT_H

// ---------------------------------------------------------------------------
// Environment.h  (part 3, phase 2)
//
// Everything that stands BEHIND the characters, drawn back to front:
//
//   dusk sky with moon and stars -> dunes and pyramids -> temple wall
//   -> the ground with its four lanes -> the lane highlight
//   -> the Sunwell Gate (home base) -> the breach -> props
//
// The layout follows the logic constants (lanes, HOME_BASE_X, ENEMY_SPAWN_X),
// so the artwork can never drift away from where the characters actually are.
// It only READS the game - which lane the cursor hovers, which lanes are
// selected - and writes triangles into the two canvases:
//
//   solid : everything drawn with normal transparency
//   light : flames and glows, uploaded with additive blending afterwards
//
// `time` (seconds) drives the flicker of the flames, the twinkling stars and
// the breathing of the lane highlight.
// ---------------------------------------------------------------------------

#include "Canvas.h"
#include "Game.h"

void drawEnvironment(const Game& game, Canvas& solid, Canvas& light,
                     float time, float viewHeight);
void drawCutsceneEnvironment(Canvas& solid, Canvas& light,
                             float time, float viewHeight);

#endif // RENDER_ENVIRONMENT_H
