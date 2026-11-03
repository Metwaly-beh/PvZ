#ifndef RENDER_CORE_H
#define RENDER_CORE_H

// ---------------------------------------------------------------------------
// Core.h
//
// The small types every visual file of part 3 shares: a colour, a vertex and
// a 2D transform. Nothing in here knows about OpenGL - art files draw into a
// Canvas (plain CPU data) and Renderer, the only GL file of part 3, uploads
// whatever the canvas holds.
//
// Coordinates are the LOGIC units of the game (see Core/Types.h):
//   x in [0, 24], y in [0, 6], y pointing UP, one unit = one unit on screen.
// ---------------------------------------------------------------------------

#include "Core/Types.h"          // Vec2, the playfield constants

// --- colour ----------------------------------------------------------------

struct Color
{
    float r, g, b, a;

    Color() : r(1.0f), g(1.0f), b(1.0f), a(1.0f) {}
    Color(float r_, float g_, float b_, float a_ = 1.0f)
        : r(r_), g(g_), b(b_), a(a_)
    {
    }
};

// Builds a colour from the 0xRRGGBB literals the palette uses.
inline Color color(unsigned int hex, float a = 1.0f)
{
    return Color(((hex >> 16) & 0xFF) / 255.0f,
                 ((hex >> 8)  & 0xFF) / 255.0f,
                 ( hex        & 0xFF) / 255.0f,
                 a);
}

inline Color mix(Color a, Color b, float t)
{
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    return Color(a.r + (b.r - a.r) * t,
                 a.g + (b.g - a.g) * t,
                 a.b + (b.b - a.b) * t,
                 a.a + (b.a - a.a) * t);
}

inline Color lighten(Color c, float t) { return mix(c, Color(1.0f, 1.0f, 1.0f), t); }
inline Color darken(Color c, float t)  { return mix(c, Color(0.0f, 0.0f, 0.0f), t); }
inline Color withAlpha(Color c, float a) { c.a = a; return c; }

// --- vertex ----------------------------------------------------------------

// Exactly what the vertex shader reads: vec3 position + vec4 colour. Keeping
// the struct identical to the attribute layout is what lets the Renderer upload
// the whole buffer with one call.
struct Vertex
{
    float x, y, z;
    float u, v;
    float r, g, b, a;

    Vertex(float x_, float y_, Color c)
        : x(x_), y(y_), z(0.0f), u(0.0f), v(0.0f), r(c.r), g(c.g), b(c.b), a(c.a)
    {
    }

    Vertex(float x_, float y_, float u_, float v_, Color c)
        : x(x_), y(y_), z(0.0f), u(u_), v(v_), r(c.r), g(c.g), b(c.b), a(c.a)
    {
    }
};

// --- 2D transform ----------------------------------------------------------

// An affine transform (move / scale / rotate / mirror), stored as
//     x' = a*x + c*y + tx
//     y' = b*x + d*y + ty
// Art code draws in its own local space (origin between the feet, +x towards
// where the character faces) and the Canvas maps that into the world.
struct Transform2D
{
    float a, b, c, d, tx, ty;

    Transform2D() : a(1.0f), b(0.0f), c(0.0f), d(1.0f), tx(0.0f), ty(0.0f) {}

    static Transform2D translate(float tx, float ty)
    {
        Transform2D t;
        t.tx = tx;
        t.ty = ty;
        return t;
    }

    static Transform2D scale(float sx, float sy)
    {
        Transform2D t;
        t.a = sx;
        t.d = sy;
        return t;
    }

    static Transform2D rotate(float radians)
    {
        Transform2D t;
        float s = std::sin(radians);
        float k = std::cos(radians);
        t.a = k;
        t.b = s;
        t.c = -s;
        t.d = k;
        return t;
    }

    // result = outer(inner(p)) - "inner" happens first.
    static Transform2D combine(const Transform2D& outer, const Transform2D& inner)
    {
        Transform2D r;
        r.a  = outer.a * inner.a + outer.c * inner.b;
        r.b  = outer.b * inner.a + outer.d * inner.b;
        r.c  = outer.a * inner.c + outer.c * inner.d;
        r.d  = outer.b * inner.c + outer.d * inner.d;
        r.tx = outer.a * inner.tx + outer.c * inner.ty + outer.tx;
        r.ty = outer.b * inner.tx + outer.d * inner.ty + outer.ty;
        return r;
    }

    void apply(float x, float y, float& outX, float& outY) const
    {
        outX = a * x + c * y + tx;
        outY = b * x + d * y + ty;
    }
};

#endif // RENDER_CORE_H
