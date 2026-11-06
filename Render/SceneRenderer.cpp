#include "SceneRenderer.h"

#include <glm/gtc/type_ptr.hpp>

#include "Assets.h"
#include "CharacterSprites.h"
#include "Environment.h"

bool SceneRenderer::init()
{
    m_time = 0.0f;
    m_atlas = 0;
    m_canvas.clear();
    m_light.clear();

    if (!m_renderer.init()) return false;

    // Everything the scene draws - lanes, props, characters - shares this
    // one texture, bound once here and re-bound by Renderer::draw().
    m_atlas = loadSpriteAtlas();
    m_renderer.bindTexture(m_atlas);
    return true;
}

void SceneRenderer::shutdown()
{
    m_renderer.shutdown();
    freeTexture(m_atlas);
    m_atlas = 0;
}

void SceneRenderer::render(const Game& game, float time, const glm::mat4& projection,
                           float viewHeight)
{
    m_time = time;
    m_canvas.clear();
    m_light.clear();
    m_renderer.setProjection(glm::value_ptr(projection));
    m_renderer.setTint(Color(1.0f, 1.0f, 1.0f, 1.0f));
    m_renderer.bindTexture(m_atlas);

    // everything that stands behind the characters, back to front
    drawEnvironment(game, m_canvas, m_light, m_time, viewHeight);

    // the characters themselves, on top of their lanes and below the
    // interface the host draws straight afterwards
    drawCharacters(game, m_canvas, m_time);

    // solid shapes first, then the glows on top of them with additive blending
    m_renderer.draw(m_canvas.vertices());
    m_renderer.draw(m_light.vertices(), BlendMode::Additive);
}
