#ifndef RENDER_SCENE_RENDERER_H
#define RENDER_SCENE_RENDERER_H

// ---------------------------------------------------------------------------
// SceneRenderer.h  (part 3 - the visual layer)
//
// Draws the whole picture that belongs to a Game: environment, characters,
// projectiles, collectibles, effects. It is called once per frame by the host
// and it only ever READS the game:
//
//     g_sceneRenderer.render(g_game, (float)glfwGetTime(), projection, viewHeight());
//
// The canvases are kept between frames so the buffers are not reallocated every
// frame, and m_time is the clock the looping animations (flames, dust, idle
// breathing) run on.
// ---------------------------------------------------------------------------

#include <glm/glm.hpp>

#include "Canvas.h"
#include "Game.h"
#include "Renderer.h"

class SceneRenderer
{
public:
    // call after gladLoadGLLoader, before the first frame
    bool init();
    void shutdown();

    // draws one frame; `projection` is the same glm::ortho the host uses and
    // `viewHeight` is the visible height in logic units (the sky fills the
    // space above the playfield)
    void render(const Game& game, float time, const glm::mat4& projection,
                float viewHeight);

private:
    Renderer m_renderer;
    Canvas   m_canvas;      // solid shapes, normal transparency
    Canvas   m_light;       // flames and glows, additive blending
    float    m_time;
    unsigned int m_atlas;   // character atlas, owned here
};

#endif // RENDER_SCENE_RENDERER_H
