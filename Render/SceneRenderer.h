#ifndef RENDER_SCENE_RENDERER_H
#define RENDER_SCENE_RENDERER_H

// ---------------------------------------------------------------------------
// SceneRenderer.h  (part 3 - the visual layer)
//
// Draws the whole picture that belongs to a Game: environment, characters,
// projectiles, collectibles, effects. It is called once per frame by the host
// and it only ever READS the game:
//
//     g_sceneRenderer.render(g_game, (float)glfwGetTime(), projection);
//
// The canvas is kept between frames so the buffer is not reallocated every
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

    // draws one frame; `projection` is the same glm::ortho the host uses
    void render(const Game& game, float time, const glm::mat4& projection);

private:
    Renderer m_renderer;
    Canvas   m_canvas;
    float    m_time;
};

#endif // RENDER_SCENE_RENDERER_H
