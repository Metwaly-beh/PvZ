#include "SceneRenderer.h"

#include <glm/gtc/type_ptr.hpp>

bool SceneRenderer::init()
{
    m_time = 0.0f;
    m_canvas.clear();
    return m_renderer.init();
}

void SceneRenderer::shutdown()
{
    m_renderer.shutdown();
}

void SceneRenderer::render(const Game& game, float time, const glm::mat4& projection)
{
    m_time = time;
    m_canvas.clear();
    m_renderer.setProjection(glm::value_ptr(projection));
    m_renderer.setTint(Color(1.0f, 1.0f, 1.0f, 1.0f));

    // Phase 1: the backend is wired up but nothing is drawn yet - the
    // environment and the characters are added from phase 2 on, each of them
    // drawing into m_canvas before the final upload.
    (void)game;
    m_renderer.draw(m_canvas.vertices());
}
