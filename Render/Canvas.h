#ifndef RENDER_CANVAS_H
#define RENDER_CANVAS_H

// ---------------------------------------------------------------------------
// Canvas.h
//
// Builds a list of coloured triangles on the CPU. All the artwork (characters,
// environment, effects) draws into a Canvas; the Renderer uploads whatever
// comes out of vertices(). That single rule is what keeps the visual code
// independent of OpenGL.
//
// Two ideas are used everywhere:
//
//   * TRANSFORMS - push() moves into a local space (a character's own origin,
//     its facing, its scale), pop() comes back out. Shapes are then written in
//     coordinates that are easy to reason about, e.g. a head at (0, 0.45).
//   * fillAndOutline() - shapes are drawn as "soft edge + outline + fill" so a
//     figure reads clearly against the sand even without multisampling, and so
//     nothing ends up as a bare primitive (section 3.1 disclaimer).
// ---------------------------------------------------------------------------

#include <vector>

#include "Core.h"
#include "Core/Types.h"      // Vec2

class Canvas
{
public:
    void clear() { m_vertices.clear(); m_transform.clear(); }
    const std::vector<Vertex>& vertices() const { return m_vertices; }

    // --- transforms -------------------------------------------------------
    void push(const Transform2D& t);
    void pop();

    // --- shapes -----------------------------------------------------------
    void quad(float x, float y, float w, float h, Color c);
    void quad(float x, float y, float w, float h, Color top, Color bottom);

    void poly(const std::vector<Vec2>& pts, Color c);
    void fillAndOutline(const std::vector<Vec2>& pts, Color fill,
                        Color outline, float width, float feather = 0.02f);

    void circle(float cx, float cy, float r, Color c, int segments = 24);
    void ellipse(float cx, float cy, float rx, float ry, float rotation,
                 Color c, int segments = 24);
    void sector(float cx, float cy, float r, float angleFrom, float angleTo,
                Color c, int segments = 12);
    void ring(float cx, float cy, float rInner, float rOuter, Color c,
              int segments = 24);
    // area inside the circle (R at cx,cy) but outside the circle (r, whose
    // centre sits `off` to the right): the moon, a curved blade, a claw
    void crescent(float cx, float cy, float R, float r, float off, Color c,
                  int segments = 24);
    void roundedRect(float x, float y, float w, float h, float radius, Color c);

    void line(Vec2 from, Vec2 to, float width, Color c);
    void polyline(const std::vector<Vec2>& pts, float width, Color c);
    void bezier(Vec2 p0, Vec2 c0, Vec2 c1, Vec2 p3, float width, Color c,
                int segments = 16);

    // Sprite rendering: draws a portion of a texture at a specific location.
    // textureId is a simple handle (GLyint), keeping this class OpenGL-agnostic.
    void drawSprite(unsigned int textureId, float sx, float sy, float sw, float sh,
                    float dx, float dy, float dw, float dh, bool flipX = false);

    // Soft radial light: flames, glows, sparks. Drawn with additive blending.
    void glow(float cx, float cy, float radius, Color c, int rings = 6);


private:
    // Every shape ends up here, with the current transform applied.
    void add(float x, float y, Color c);
    void tri(Vec2 a, Vec2 b, Vec2 c, Color ca, Color cb, Color cc);
    void band(const std::vector<Vec2>& outer, const std::vector<Vec2>& inner,
              Color outerColor, Color innerColor);

    std::vector<Vertex>      m_vertices;
    std::vector<Transform2D> m_transform;   // empty means "world space"
};

#endif // RENDER_CANVAS_H
