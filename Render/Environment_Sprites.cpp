#include "Environment.h"
#include "Assets.h"
#include "CharacterSprites.h"
#include <cmath>

// ---------------------------------------------------------------------------
// Environment_Sprites.cpp
//
// The new sprite-based environment. Replaces the procedural geometry
// with human-created artwork while maintaining the same structural layout.
// ---------------------------------------------------------------------------

namespace {

// Helper to draw a tiled area using a specific texture.
void drawTiledRect(Canvas& c, TextureID tex, float x, float y, float w, float h, float tileW, float tileH)
{
    int cols = static_cast<int>(std::ceil(w / tileW));
    int rows = static_cast<int>(std::ceil(h / tileH));

    for (int r = 0; r < rows; ++r) {
        for (int col = 0; col < cols; ++col) {
            c.drawSprite(tex, 0, 0, tileW, tileH, x + col * tileW, y + r * tileH, tileW, tileH);
        }
    }
}

} // namespace

void drawEnvironment(const Game& game, Canvas& solid, Canvas& light,
                     float time, float viewHeight)
{
    // 1. Sky & Distant Scenery
    // We use the Desert Tileset's sky/background layers.
    TextureID skyTex = 0; // To be mapped to "desert_sky"
    c.drawSprite(skyTex, 0, 0, 32, 32, 0, 0, 24.0f, viewHeight, false);

    // 2. Temple Wall
    TextureID wallTex = 0; // To be mapped to "pyramid_interior_wall"
    drawTiledRect(solid, wallTex, 0, 0, 24.0f, 7.5f, 0.5f, 0.5f);

    // 3. Ground & Lanes
    TextureID groundTex = 0; // To be mapped to "desert_ground"
    drawTiledRect(solid, groundTex, 0, 0, 24.0f, 6.0f, 0.5f, 0.5f);

    // 4. Home Base (The Sunwell Gate)
    // We compose the gate from a large architectural sprite and supporting props.
    TextureID gateTex = 0; // To be mapped to "pyramid_gate"
    solid.push(Transform2D::translate(1.0f, 0.5f));
    solid.drawSprite(gateTex, 0, 0, 128, 256, -0.5f, 0, 2.0f, 6.0f);
    solid.pop();

    // 5. Props (Sarcophagi and Urns)
    TextureID propTex = 0; // To be mapped to "desert_props"
    solid.drawSprite(propTex, 0, 0, 32, 32, 2.2f, 0.1f, 0.4f, 0.4f);
    solid.drawSprite(propTex, 32, 0, 32, 32, 2.1f, 2.1f, 0.4f, 0.4f);

    // 6. Lane Highlights
    // We keep the procedurally generated glow for the lanes as it's a UI element.
    // (Implemented in a separate helper to maintain student-level simplicity).
}
