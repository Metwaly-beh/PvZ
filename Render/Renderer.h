#ifndef RENDER_RENDERER_H
#define RENDER_RENDERER_H

// ---------------------------------------------------------------------------
// Renderer.h
//
// The ONLY file of part 3 that talks to OpenGL. Art files draw into a Canvas
// (plain CPU data) and this class uploads the result, so Part 1 can replace
// the backend later without touching a single visual file.
//
// This header includes neither glad nor glm on purpose - the projection is
// handed over as the 16 floats of a column-major matrix (glm::value_ptr),
// which is what keeps the rest of the rendering code free of GL types.
//
//   Renderer r;
//   r.init();                       // after gladLoadGLLoader
//   r.setProjection(glm::value_ptr(projection));
//   r.draw(canvas.vertices());      // one call per layer
//   r.shutdown();
// ---------------------------------------------------------------------------

#include <vector>

#include "Core.h"

enum class BlendMode
{
    Alpha,      // normal transparency (everything that is not light)
    Additive    // light: flames, glows, sparks (glow + colour, never darker)
};

class Renderer
{
public:
    Renderer();

    // builds the shader program and the buffers; false on shader failure
    bool init();
    void shutdown();

    void setProjection(const float* matrix16);   // column-major, glm::value_ptr
    void setTint(Color c);                       // global colour grade

    void draw(const std::vector<Vertex>& vertices,
              BlendMode mode = BlendMode::Alpha);

    // Selects the texture every following draw() samples. Pass 0 to go back
    // to flat colours. The scene binds the character atlas once per frame.
    void bindTexture(unsigned int textureId);

private:
    unsigned int m_program;
    unsigned int m_vao;
    unsigned int m_vbo;
    unsigned int m_texture;   // 0 = flat colours, otherwise the bound atlas
    int          m_uProjection;
    int          m_uTint;
    int          m_uTexture;
    int          m_uUseTexture;
    float        m_projection[16];
    Color        m_tint;
};

#endif // RENDER_RENDERER_H
