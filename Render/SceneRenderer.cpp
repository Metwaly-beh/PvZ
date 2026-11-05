#include "SceneRenderer.h"

#include <glm/gtc/type_ptr.hpp>

#include "Environment.h"

bool SceneRenderer::init()
{
    m_time = 0.0f;
    m_canvas.clear();
    m_light.clear();
    return m_renderer.init();
}

void SceneRenderer::shutdown()
{
    m_renderer.shutdown();
}

void SceneRenderer::render(const Game& game, float time, const glm::mat4& projection,
                           float viewHeight)
{
    m_time = time;
    m_canvas.clear();
    m_light.clear();
    m_renderer.setProjection(glm::value_ptr(projection));
    m_renderer.setTint(Color(1.0f, 1.0f, 1.0f, 1.0f));

    // everything that stands behind the characters, back to front
    drawEnvironment(game, m_canvas, m_light, m_time, viewHeight);

    // solid shapes first, then the glows on top of them with additive blending
    m_renderer.draw(m_canvas.vertices());
    m_renderer.draw(m_light.vertices(), BlendMode::Additive);
}
