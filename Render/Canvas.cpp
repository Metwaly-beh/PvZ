#include "Canvas.h"

#include <algorithm>
#include <cmath>

// ---------------------------------------------------------------------------
// Small geometry helpers. Everything the shapes need and nothing more:
// signed area, point-in-triangle, ear clipping and polygon offsetting.
// ---------------------------------------------------------------------------

namespace {

float cross2(const Vec2& a, const Vec2& b)
{
    return a.x * b.y - a.y * b.x;
}

float signedArea(const std::vector<Vec2>& pts)
{
    float area = 0.0f;
    for (size_t i = 0; i < pts.size(); ++i) {
        const Vec2& a = pts[i];
        const Vec2& b = pts[(i + 1) % pts.size()];
        area += a.x * b.y - b.x * a.y;
    }
    return 0.5f * area;
}

bool pointInTriangle(const Vec2& p, const Vec2& a, const Vec2& b, const Vec2& c)
{
    float d1 = cross2(p - a, b - a);
    float d2 = cross2(p - b, c - b);
    float d3 = cross2(p - c, a - c);
    bool hasNeg = (d1 < 0.0f) || (d2 < 0.0f) || (d3 < 0.0f);
    bool hasPos = (d1 > 0.0f) || (d2 > 0.0f) || (d3 > 0.0f);
    return !(hasNeg && hasPos);
}

bool isEar(const std::vector<Vec2>& pts, const std::vector<int>& remaining,
           int ia, int ib, int ic)
{
    const Vec2& a = pts[ia];
    const Vec2& b = pts[ib];
    const Vec2& c = pts[ic];

    if (cross2(b - a, c - b) <= 1e-6f) return false;

    for (size_t k = 0; k < remaining.size(); ++k) {
        int i = remaining[k];
        if (i == ia || i == ib || i == ic) continue;
        if (pointInTriangle(pts[i], a, b, c)) return false;
    }
    return true;
}

std::vector<int> triangulate(const std::vector<Vec2>& pts)
{
    std::vector<int> remaining;
    remaining.reserve(pts.size());
    for (size_t i = 0; i < pts.size(); ++i) {
        const Vec2& p = pts[i];
        const Vec2& prev = pts[(i + pts.size() - 1) % pts.size()];
        if (std::fabs(p.x - prev.x) < 1e-4f && std::fabs(p.y - prev.y) < 1e-4f)
            continue;
        remaining.push_back(static_cast<int>(i));
    }
    if (remaining.size() < 3) return std::vector<int>();

    if (signedArea(pts) < 0.0f)
        std::reverse(remaining.begin(), remaining.end());

    std::vector<int> tris;
    size_t guard = 0;
    size_t limit = pts.size() * pts.size() + 8;

    while (remaining.size() > 3 && guard++ < limit) {
        bool clipped = false;
        for (size_t i = 0; i < remaining.size(); ++i) {
            int ia = remaining[(i + remaining.size() - 1) % remaining.size()];
            int ib = remaining[i];
            int ic = remaining[(i + 1) % remaining.size()];
            if (!isEar(pts, remaining, ia, ib, ic)) continue;

            tris.push_back(ia);
            tris.push_back(ib);
            tris.push_back(ic);
            remaining.erase(remaining.begin() + i);
            clipped = true;
            break;
        }
        if (!clipped) break;
    }

    for (size_t i = 1; i + 1 < remaining.size(); ++i) {
        tris.push_back(remaining[0]);
        tris.push_back(remaining[i]);
        tris.push_back(remaining[i + 1]);
    }
    return tris;
}

std::vector<Vec2> offsetPolygon(const std::vector<Vec2>& pts, float width)
{
    const size_t n = pts.size();
    std::vector<Vec2> out(n);

    for (size_t i = 0; i < n; ++i) {
        const Vec2& prev = pts[(i + n - 1) % n];
        const Vec2& cur  = pts[i];
        const Vec2& next = pts[(i + 1) % n];

        Vec2 d1 = cur - prev;
        Vec2 d2 = next - cur;
        Vec2 n1(d1.y, -d1.x);
        Vec2 n2(d2.y, -d2.x);
        Vec2 n = n1 + n2;

        float len = std::sqrt(n.x * n.x + n.y * n.y);
        if (len < 1e-6f) {
            out[i] = cur;
            continue;
        }
        n = n * (1.0f / len);

        float limit = 0.5f * std::min(std::sqrt(d1.x * d1.x + d1.y * d1.y),
                                      std::sqrt(d2.x * d2.x + d2.y * d2.y));
        float step = std::min(width, limit);
        out[i] = Vec2(cur.x + n.x * step, cur.y + n.y * step);
    }
    return out;
}

std::vector<Vec2> circleLoop(float cx, float cy, float r, int segments)
{
    std::vector<Vec2> pts;
    pts.reserve(static_cast<size_t>(segments));
    for (int i = 0; i < segments; ++i) {
        float angle = 6.2831853f * static_cast<float>(i) / static_cast<float>(segments);
        pts.push_back(Vec2(cx + r * std::cos(angle), cy + r * std::sin(angle)));
    }
    return pts;
}

} // namespace

void Canvas::push(const Transform2D& t)
{
    if (m_transform.empty()) m_transform.push_back(t);
    else m_transform.push_back(Transform2D::combine(m_transform.back(), t));
}

void Canvas::pop()
{
    if (!m_transform.empty()) m_transform.pop_back();
}

void Canvas::add(float x, float y, Color c)
{
    if (!m_transform.empty()) {
        float ox = x, oy = y;
        m_transform.back().apply(x, y, ox, oy);
        x = ox;
        y = oy;
    }
    m_vertices.push_back(Vertex(x, y, c));
}

void Canvas::tri(Vec2 a, Vec2 b, Vec2 c, Color ca, Color cb, Color cc)
{
    add(a.x, a.y, ca);
    add(b.x, b.y, cb);
    add(c.x, c.y, cc);
}

void Canvas::quad(float x, float y, float w, float h, Color c)
{
    add(x, y, c);
    add(x + w, y, c);
    add(x + w, y + h, c);

    add(x, y, c);
    add(x + w, y + h, c);
    add(x, y + h, c);
}

void Canvas::quad(float x, float y, float w, float h, Color top, Color bottom)
{
    add(x, y, bottom);
    add(x + w, y, bottom);
    add(x + w, y + h, top);

    add(x, y, bottom);
    add(x + w, y + h, top);
    add(x, y + h, top);
}

void Canvas::poly(const std::vector<Vec2>& pts, Color c)
{
    if (pts.size() < 3) return;

    std::vector<int> tris = triangulate(pts);
    for (size_t i = 0; i + 2 < tris.size(); i += 3) {
        tri(pts[tris[i]], pts[tris[i + 1]], pts[tris[i + 2]], c, c, c);
    }
}

void Canvas::band(const std::vector<Vec2>& outer, const std::vector<Vec2>& inner,
                  Color outerColor, Color innerColor)
{
    size_t n = outer.size();
    if (inner.size() != n || n < 2) return;

    for (size_t i = 0; i < n; ++i) {
        size_t j = (i + 1) % n;
        tri(outer[i], outer[j], inner[j], outerColor, outerColor, innerColor);
        tri(outer[i], inner[j], inner[i], outerColor, innerColor, innerColor);
    }
}

void Canvas::fillAndOutline(const std::vector<Vec2>& pts, Color fill,
                            Color outline, float width, float feather)
{
    if (pts.size() < 3) return;

    std::vector<Vec2> shape = pts;
    if (signedArea(shape) < 0.0f) std::reverse(shape.begin(), shape.end());

    if (feather > 0.0f) {
        band(offsetPolygon(shape, width + feather),
             offsetPolygon(shape, width),
             withAlpha(outline, 0.0f), outline);
    }
    band(offsetPolygon(shape, width), shape, outline, outline);
    poly(shape, fill);
}

void Canvas::circle(float cx, float cy, float r, Color c, int segments)
{
    poly(circleLoop(cx, cy, r, segments), c);
}

void Canvas::ellipse(float cx, float cy, float rx, float ry, float rotation,
                     Color c, int segments)
{
    std::vector<Vec2> pts;
    float s = std::sin(rotation);
    float k = std::cos(rotation);

    for (int i = 0; i < segments; ++i) {
        float angle = 6.2831853f * static_cast<float>(i) / static_cast<float>(segments);
        float px = rx * std::cos(angle);
        float py = ry * std::sin(angle);
        pts.push_back(Vec2(cx + px * k - py * s, cy + px * s + py * k));
    }
    poly(pts, c);
}

void Canvas::sector(float cx, float cy, float r, float angleFrom, float angleTo,
                     Color c, int segments)
{
    std::vector<Vec2> pts;
    pts.push_back(Vec2(cx, cy));
    for (int i = 0; i <= segments; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(segments);
        float angle = angleFrom + (angleTo - angleFrom) * t;
        pts.push_back(Vec2(cx + r * std::cos(angle), cy + r * std::sin(angle)));
    }
    poly(pts, c);
}

void Canvas::ring(float cx, float cy, float rInner, float rOuter, Color c,
                  int segments)
{
    band(circleLoop(cx, cy, rOuter, segments),
         circleLoop(cx, cy, rInner, segments), c, c);
}

void Canvas::crescent(float cx, float cy, float R, float r, float off, Color c,
                      int segments)
{
    if (off <= 0.0f) {
        circle(cx, cy, R, c, segments);
        return;
    }
    float a  = (off * off + R * R - r * r) / (2.0f * off);
    float h2 = R * R - a * a;
    if (h2 <= 0.0f) {
        circle(cx, cy, R, c, segments);
        return;
    }
    float h = std::sqrt(h2);

    float outerFrom = std::atan2(h, a);
    float outerTo   = 6.2831853f - outerFrom;
    float innerFrom = 6.2831853f - std::atan2(h, a - off);
    float innerTo   = std::atan2(h, a - off);

    std::vector<Vec2> pts;
    pts.reserve(static_cast<size_t>(2 * segments));
    for (int i = 0; i <= segments; ++i) {
        float t = outerFrom + (outerTo - outerFrom) * static_cast<float>(i) / segments;
        pts.push_back(Vec2(cx + R * std::cos(t), cy + R * std::sin(t)));
    }
    for (int i = 1; i < segments; ++i) {
        float t = innerFrom + (innerTo - innerFrom) * static_cast<float>(i) / segments;
        pts.push_back(Vec2(cx + off + r * std::cos(t), cy + r * std::sin(t)));
    }
    poly(pts, c);
}

void Canvas::roundedRect(float x, float y, float w, float h, float radius, Color c)
{
    float r = radius;
    if (r > 0.5f * w) r = 0.5f * w;
    if (r > 0.5f * h) r = 0.5f * h;

    if (r <= 0.0f) {
        quad(x, y, w, h, c);
        return;
    }

    const int cornerSegments = 4;
    std::vector<Vec2> pts;
    for (int corner = 0; corner < 4; ++corner) {
        float cx = x + r + ((corner == 1 || corner == 2) ? (w - 2.0f * r) : 0.0f);
        float cy = y + r + ((corner == 2 || corner == 3) ? (h - 2.0f * r) : 0.0f);
        for (int i = 0; i <= cornerSegments; ++i) {
            float deg = 180.0f + 90.0f * corner +
                        90.0f * static_cast<float>(i) / static_cast<float>(cornerSegments);
            float angle = deg * 0.017453293f;
            pts.push_back(Vec2(cx + r * std::cos(angle), cy + r * std::sin(angle)));
        }
    }
    pts.pop_back();
    poly(pts, c);
}

void Canvas::line(Vec2 from, Vec2 to, float width, Color c)
{
    Vec2 d = to - from;
    float len = std::sqrt(d.x * d.x + d.y * d.y);
    if (len < 1e-5f) {
        circle(from.x, from.y, width * 0.5f, c, 12);
        return;
    }

    Vec2 n(d.y / len * (width * 0.5f), -d.x / len * (width * 0.5f));
    std::vector<Vec2> pts;
    pts.push_back(Vec2(from.x + n.x, from.y + n.y));
    pts.push_back(Vec2(to.x + n.x, to.y + n.y));
    pts.push_back(Vec2(to.x - n.x, to.y - n.y));
    pts.push_back(Vec2(from.x - n.x, from.y - n.y));
    poly(pts, c);

    circle(from.x, from.y, width * 0.5f, c, 10);
    circle(to.x, to.y, width * 0.5f, c, 10);
}

void Canvas::polyline(const std::vector<Vec2>& pts, float width, Color c)
{
    for (size_t i = 0; i + 1 < pts.size(); ++i) line(pts[i], pts[i + 1], width, c);
}

void Canvas::bezier(Vec2 p0, Vec2 c0, Vec2 c1, Vec2 p3, float width, Color c,
                    int segments)
{
    std::vector<Vec2> pts;
    for (int i = 0; i <= segments; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(segments);
        float u = 1.0f - t;
        float b0 = u * u * u;
        float b1 = 3.0f * u * u * t;
        float b2 = 3.0f * u * t * t;
        float b3 = t * t * t;
        pts.push_back(Vec2(b0 * p0.x + b1 * c0.x + b2 * c1.x + b3 * p3.x,
                           b0 * p0.y + b1 * c0.y + b2 * c1.y + b3 * p3.y));
    }
    polyline(pts, width, c);
}

void Canvas::glow(float cx, float cy, float radius, Color c, int rings)
{
    for (int i = 0; i < rings; ++i) {
        float t0 = static_cast<float>(i) / static_cast<float>(rings);
        float t1 = static_cast<float>(i + 1) / static_cast<float>(rings);
        float a0 = c.a * (1.0f - t0) * (1.0f - t0);
        float a1 = c.a * (1.0f - t1) * (1.0f - t1);
        band(circleLoop(cx, cy, radius * t1, 16),
             circleLoop(cx, cy, radius * t0, 16),
             withAlpha(c, a1), withAlpha(c, a0));
    }
}

void Canvas::drawSprite(unsigned int textureId, float sx, float sy, float sw, float sh,
                float dx, float dy, float dw, float dh, bool flipX)
{
    // We will use a specific color (white) to tell the renderer that this is a sprite.
    // However, a better way for a student project is to just add the vertices with UVs.
    // The current Renderer already uses u_UseTexture, so we just need to pass that.
    // Since we can't pass a per-vertex uniform, we'll just add the quad and 
    // we'll handle the "use texture" state in the renderer by grouping.
    // For now, let's just add the vertices with correct UVs.
    
    float u0 = sx / sw;
    float v0 = sy / sh;
    float u1 = (sx + sw) / sw;
    float v1 = (sy + sh) / sh;
    
    if (flipX) std::swap(u0, u1);

    Color white(1.0f, 1.0f, 1.0f, 1.0f);
    
    // Triangle 1
    m_vertices.push_back(Vertex(dx, dy, u0, v0, white));
    m_vertices.push_back(Vertex(dx + dw, dy, u1, v0, white));
    m_vertices.push_back(Vertex(dx + dw, dy + dh, u1, v1, white));
    
    // Triangle 2
    m_vertices.push_back(Vertex(dx, dy, u0, v0, white));
    m_vertices.push_back(Vertex(dx + dw, dy + dh, u1, v1, white));
    m_vertices.push_back(Vertex(dx, dy + dh, u0, v1, white));
}
