#include "Environment.h"

#include <cmath>

#include "Palette.h"

// ---------------------------------------------------------------------------
// The courtyard of "The Sealed Tomb", back to front:
//
//   sky -> dunes & pyramids -> temple wall -> ground & lanes -> lane highlight
//       -> Sunwell Gate -> breach -> props
//
// Everything is built from layered shapes with outlines and shading, never
// from a bare rectangle, so the scene reads as drawn artwork (section 3.1).
// ---------------------------------------------------------------------------

namespace {

// the wall that closes the courtyard at the back; the dunes behind it start
// at the same line so their feet are hidden by the wall
const float WALL_TOP = 7.5f;

// --- small helpers ---------------------------------------------------------

// Deterministic 0..1 from an integer: stars, cracks and rubble stay in the
// same place on every run and every frame, without needing a random generator.
float hash01(int i)
{
    float x = std::sin(static_cast<float>(i) * 12.9898f) * 43758.5453f;
    return x - std::floor(x);
}

// A pyramid from the side: shaded left face, moonlit right face.
void pyramid(Canvas& c, float tipX, float baseY, float halfWidth, float height,
             Color shade, Color lit)
{
    c.poly({Vec2(tipX, baseY + height), Vec2(tipX - halfWidth, baseY),
            Vec2(tipX, baseY)}, shade);
    c.poly({Vec2(tipX, baseY + height), Vec2(tipX, baseY),
            Vec2(tipX + halfWidth, baseY)}, lit);
}

// One flickering flame: two nested tongues plus the light it throws.
void flame(Canvas& solid, Canvas& light, float x, float y, float size,
           float time, float phase)
{
    float flicker = 0.85f + 0.15f * std::sin(time * 9.0f + phase) +
                    0.07f * std::sin(time * 23.0f + phase * 2.0f);
    float sway = 0.12f * size * std::sin(time * 5.0f + phase);
    float h = 2.1f * size * flicker;

    solid.poly({Vec2(x - 0.45f * size, y),
                Vec2(x - 0.50f * size, y + h * 0.45f),
                Vec2(x - 0.16f * size + sway, y + h),
                Vec2(x + 0.18f * size + sway, y + h * 0.84f),
                Vec2(x + 0.50f * size, y + h * 0.40f),
                Vec2(x + 0.45f * size, y)}, FLAME);

    solid.poly({Vec2(x - 0.24f * size, y),
                Vec2(x - 0.22f * size, y + h * 0.40f),
                Vec2(x + 0.04f * size + sway, y + h * 0.66f),
                Vec2(x + 0.24f * size, y + h * 0.34f),
                Vec2(x + 0.24f * size, y)}, FLAME_CORE);

    light.glow(x, y + h * 0.45f, 3.0f * size,
               withAlpha(FLAME, 0.55f * flicker), 5);
}

// A standing bronze brazier: tripod legs, a bowl, the fire on top.
void brazier(Canvas& solid, Canvas& light, float x, float floorY,
             float time, float phase)
{
    solid.line(Vec2(x - 0.24f, floorY), Vec2(x - 0.07f, floorY + 0.34f),
               0.07f, BRONZE_DARK);
    solid.line(Vec2(x + 0.24f, floorY), Vec2(x + 0.07f, floorY + 0.34f),
               0.07f, BRONZE_DARK);
    solid.line(Vec2(x, floorY - 0.02f), Vec2(x, floorY + 0.34f),
               0.06f, darken(BRONZE_DARK, 0.35f));

    solid.fillAndOutline({Vec2(x - 0.34f, floorY + 0.30f),
                          Vec2(x + 0.34f, floorY + 0.30f),
                          Vec2(x + 0.25f, floorY + 0.56f),
                          Vec2(x - 0.25f, floorY + 0.56f)},
                         BRONZE, OUTLINE_PLAYER, 0.035f);
    solid.quad(x - 0.38f, floorY + 0.54f, 0.76f, 0.09f, BRONZE_LIGHT, BRONZE);

    flame(solid, light, x, floorY + 0.62f, 0.30f, time, phase);
}

// One of four hieroglyphs carved into the wall panels.
void glyph(Canvas& c, int kind, float x, float y, float s, Color col)
{
    switch (kind % 4) {
    case 0:      // eye: ring, pupil, tear
        c.ring(x, y, 0.55f * s, 0.85f * s, col, 16);
        c.circle(x, y, 0.30f * s, col, 12);
        c.line(Vec2(x + 0.75f * s, y - 0.25f * s),
               Vec2(x + 1.15f * s, y - 0.75f * s), 0.16f * s, col);
        break;
    case 1:      // ankh
        c.ring(x, y + 0.75f * s, 0.35f * s, 0.60f * s, col, 14);
        c.line(Vec2(x, y + 0.20f * s), Vec2(x, y - 1.10f * s), 0.20f * s, col);
        c.line(Vec2(x - 0.80f * s, y + 0.10f * s),
               Vec2(x + 0.80f * s, y + 0.10f * s), 0.20f * s, col);
        break;
    case 2:      // water ripple
        c.polyline({Vec2(x - 1.10f * s, y + 0.40f * s), Vec2(x - 0.55f * s, y - 0.30f * s),
                    Vec2(x, y + 0.40f * s), Vec2(x + 0.55f * s, y - 0.30f * s),
                    Vec2(x + 1.10f * s, y + 0.40f * s)}, 0.18f * s, col);
        break;
    default:     // bird
        c.circle(x - 1.05f * s, y - 0.45f * s, 0.24f * s, col, 10);
        c.polyline({Vec2(x - 0.95f * s, y - 0.45f * s), Vec2(x - 0.15f * s, y - 0.50f * s),
                    Vec2(x + 0.55f * s, y - 0.05f * s), Vec2(x + 1.05f * s, y - 0.55f * s)},
                   0.20f * s, col);
        c.polyline({Vec2(x - 0.10f * s, y - 0.50f * s), Vec2(x + 0.25f * s, y + 0.50f * s),
                    Vec2(x + 0.65f * s, y - 0.45f * s)}, 0.18f * s, col);
        break;
    }
}

// A clay urn standing on the given floor line.
void urn(Canvas& c, float x, float floorY, float scale)
{
    float w = 0.34f * scale;
    float h = 0.55f * scale;

    c.ellipse(x, floorY + h * 0.45f, w, h * 0.55f, 0.0f, TERRACOTTA, 20);
    c.quad(x - w * 0.34f, floorY + h * 0.82f, w * 0.68f, h * 0.32f,
           lighten(TERRACOTTA, 0.15f), TERRACOTTA);
    c.quad(x - w * 0.55f, floorY + h * 1.08f, w * 1.10f, h * 0.16f,
           lighten(TERRACOTTA, 0.30f), TERRACOTTA);
    c.line(Vec2(x - w * 1.02f, floorY + h * 0.52f),
           Vec2(x - w * 0.45f, floorY + h * 0.92f), 0.05f * scale,
           darken(TERRACOTTA, 0.30f));
    c.line(Vec2(x + w * 1.02f, floorY + h * 0.52f),
           Vec2(x + w * 0.45f, floorY + h * 0.92f), 0.05f * scale,
           darken(TERRACOTTA, 0.30f));

    // the moonlight catching one side keeps it from reading as a flat blob
    c.ellipse(x - w * 0.35f, floorY + h * 0.50f, w * 0.26f, h * 0.34f,
              -0.4f, withAlpha(lighten(TERRACOTTA, 0.40f), 0.55f), 12);
}

// A scatter of bones, pot shards and pebbles on a floor line.
void debris(Canvas& c, float x, float floorY, int seed)
{
    for (int i = 0; i < 5; ++i) {
        float px = x + (hash01(seed + i) - 0.5f) * 1.4f;
        float py = floorY + 0.05f + 0.08f * hash01(seed + i + 30);
        float kind = hash01(seed + i + 60);

        if (kind < 0.4f) {                      // a bone
            float len = 0.16f + 0.14f * hash01(seed + i + 90);
            float ang = (hash01(seed + i + 120) - 0.5f) * 0.9f;
            Vec2 a(px, py);
            Vec2 b(px + len * std::cos(ang), py + len * std::sin(ang));
            c.line(a, b, 0.055f, BONE);
            c.circle(a.x, a.y, 0.048f, BONE, 8);
            c.circle(b.x, b.y, 0.048f, BONE, 8);
        } else if (kind < 0.7f) {               // a pot shard
            c.poly({Vec2(px, py), Vec2(px + 0.20f, py + 0.03f),
                    Vec2(px + 0.11f, py + 0.15f)}, TERRACOTTA);
        } else {                                // a pebble
            c.ellipse(px, py, 0.08f, 0.05f, 0.0f, SAND_DARK, 8);
        }
    }
}

// --- the layers ------------------------------------------------------------

void drawSky(Canvas& solid, Canvas& light, float time, float viewHeight)
{
    const float horizon = PLAYFIELD_HEIGHT;
    if (viewHeight <= horizon + 0.05f) return;          // no band to fill

    // indigo overhead -> violet -> dusty rose where it meets the wall
    float mid = horizon + 0.45f * (viewHeight - horizon);
    solid.quad(0.0f, mid, PLAYFIELD_WIDTH, viewHeight - mid, SKY_TOP, SKY_MID);
    solid.quad(0.0f, horizon, PLAYFIELD_WIDTH, mid - horizon, SKY_MID, SKY_HORIZON);

    // stars, only above the haze, each twinkling on its own rhythm
    for (int i = 0; i < 46; ++i) {
        float x = hash01(i * 3 + 1) * PLAYFIELD_WIDTH;
        float y = mid + 0.35f + hash01(i * 3 + 2) * (viewHeight - mid - 0.6f);
        if (y >= viewHeight - 0.15f) continue;
        float twinkle = 0.5f + 0.5f * std::sin(time * (1.2f + 2.5f * hash01(i + 71)) + i);
        float r = 0.022f + 0.030f * hash01(i + 131);
        solid.circle(x, y, r, withAlpha(STAR, 0.25f + 0.65f * twinkle), 6);
    }

    // slow dust motes drifting through the moon light
    for (int i = 0; i < 14; ++i) {
        float x = hash01(i * 5 + 3) * PLAYFIELD_WIDTH;
        x = std::fmod(x + time * (0.12f + 0.25f * hash01(i + 17)), PLAYFIELD_WIDTH);
        float y = horizon + 0.6f + hash01(i * 5 + 4) * (viewHeight - horizon - 1.4f);
        light.circle(x, y, 0.035f, withAlpha(STAR, 0.08f + 0.10f * hash01(i + 5)), 6);
    }

    // the moon and its halo, clear of the gate on the right
    float moonX = 19.4f;
    float moonY = horizon + 0.72f * (viewHeight - horizon);
    light.glow(moonX, moonY, 1.9f, withAlpha(MOON_GLOW, 0.30f), 6);
    solid.crescent(moonX, moonY, 0.58f, 0.44f, 0.26f, MOON);
}

void drawDistantScenery(Canvas& solid)
{
    // dunes first - flat, hazier - then the pyramids standing on the same line
    solid.ellipse(5.5f, WALL_TOP, 7.5f, 1.5f, 0.0f, DUNE_FAR, 28);
    solid.ellipse(17.0f, WALL_TOP, 8.5f, 1.9f, 0.0f, DUNE_NEAR, 28);
    solid.ellipse(11.0f, WALL_TOP, 6.0f, 1.1f, 0.0f,
                  mix(DUNE_FAR, DUNE_NEAR, 0.5f), 28);

    pyramid(solid, 4.0f, WALL_TOP, 2.1f, 1.75f, PYRAMID_FAR,
            mix(PYRAMID_FAR, SKY_HORIZON, 0.40f));
    pyramid(solid, 9.4f, WALL_TOP, 1.4f, 1.15f, PYRAMID_FAR,
            mix(PYRAMID_FAR, SKY_HORIZON, 0.32f));
    pyramid(solid, 15.6f, WALL_TOP, 2.8f, 2.35f, PYRAMID_NEAR,
            mix(PYRAMID_NEAR, SKY_HORIZON, 0.45f));
}

void drawTempleWall(Canvas& solid)
{
    const float y0 = PLAYFIELD_HEIGHT;
    const float y1 = WALL_TOP;

    // the face: moonlit near the top, in shadow where it meets the floor
    solid.quad(0.0f, y0, PLAYFIELD_WIDTH, y1 - y0,
               mix(WALL, WALL_LIGHT, 0.55f), WALL);
    solid.quad(0.0f, y0, PLAYFIELD_WIDTH, 0.40f,
               WALL, mix(WALL, WALL_SHADOW, 0.75f));

    // cornice and the shadow it throws
    solid.quad(0.0f, y1 - 0.34f, PLAYFIELD_WIDTH, 0.34f,
               WALL_LIGHT, mix(WALL, WALL_LIGHT, 0.35f));
    solid.quad(0.0f, y1 - 0.44f, PLAYFIELD_WIDTH, 0.10f,
               withAlpha(WALL_SHADOW, 0.85f), withAlpha(WALL_SHADOW, 0.30f));

    // pilasters: lit edge, shaded side, capital and base - the wall gets depth
    for (int i = 0; i < 8; ++i) {
        float x = 0.6f + 3.0f * i;
        float shaftY = y0 + 0.44f;
        float shaftH = (y1 - 0.86f) - shaftY;

        solid.quad(x, shaftY, 0.64f, shaftH,
                   mix(WALL, WALL_LIGHT, 0.35f), mix(WALL, WALL_SHADOW, 0.20f));
        solid.quad(x, shaftY, 0.13f, shaftH,
                   mix(WALL_LIGHT, STONE_LIGHT, 0.45f), mix(WALL, WALL_LIGHT, 0.45f));
        solid.quad(x + 0.51f, shaftY, 0.13f, shaftH,
                   WALL_SHADOW, mix(WALL_SHADOW, WALL, 0.45f));

        solid.quad(x - 0.07f, y1 - 0.86f, 0.78f, 0.22f, WALL_LIGHT, WALL);
        solid.quad(x - 0.07f, shaftY - 0.17f, 0.78f, 0.17f,
                   mix(WALL, WALL_SHADOW, 0.35f), mix(WALL, WALL_SHADOW, 0.70f));
    }

    // recessed panels with a hieroglyph each, between the pilasters
    for (int i = 0; i < 7; ++i) {
        float x = 2.42f + 3.0f * i;
        solid.quad(x - 0.85f, y0 + 0.62f, 1.7f, 0.66f,
                   mix(WALL, WALL_SHADOW, 0.45f), mix(WALL, WALL_LIGHT, 0.25f));
        glyph(solid, i, x, y0 + 0.95f, 0.24f, WALL_GLYPH);
    }

    // a few cracks so the wall does not look tiled
    for (int i = 0; i < 5; ++i) {
        float x = 2.0f + hash01(i + 400) * 20.0f;
        float y = y0 + 0.5f + hash01(i + 500) * (y1 - y0 - 1.1f);
        solid.polyline({Vec2(x, y), Vec2(x + 0.18f, y + 0.22f),
                        Vec2(x + 0.05f, y + 0.44f)},
                       0.035f, withAlpha(WALL_SHADOW, 0.70f));
    }
}

void drawGround(Canvas& solid)
{
    // base floor: shaded where the wall blocks the sky, dark at the front edge
    solid.quad(0.0f, 0.0f, PLAYFIELD_WIDTH, PLAYFIELD_HEIGHT,
               mix(SAND, WALL_SHADOW, 0.40f), SAND_DARK);

    for (int lane = 0; lane < LANE_COUNT; ++lane) {
        float y0 = lane * LANE_HEIGHT;
        float centerY = y0 + 0.5f * LANE_HEIGHT;

        // each lane is its own slab - alternate the tint so four lanes read apart
        Color slab = (lane % 2 == 0) ? mix(SAND, SAND_LIGHT, 0.55f)
                                     : mix(SAND, STONE_SHADE, 0.35f);
        solid.quad(0.0f, y0 + 0.10f, PLAYFIELD_WIDTH, LANE_HEIGHT - 0.20f,
                   withAlpha(slab, 0.60f));

        // the worn path characters walk down the middle of the slab
        solid.quad(0.0f, centerY - 0.40f, PLAYFIELD_WIDTH, 0.80f,
                   withAlpha(LANE_WORN, 0.30f));
        solid.quad(0.0f, centerY - 0.40f, PLAYFIELD_WIDTH, 0.05f,
                   withAlpha(SAND_DARK, 0.35f));
        solid.quad(0.0f, centerY + 0.35f, PLAYFIELD_WIDTH, 0.05f,
                   withAlpha(SAND_DARK, 0.30f));

        // brick joints, offset on the alternate rows
        for (int k = 0; k < 12; ++k) {
            float x = 1.1f + 2.0f * k + ((lane % 2 == 0) ? 0.0f : 1.0f);
            solid.quad(x, y0 + 0.16f, 0.035f, LANE_HEIGHT - 0.32f,
                       withAlpha(OUTLINE_DARK, 0.16f));
        }

        // one or two cracks, placed by the hash so they never move
        for (int k = 0; k < 2; ++k) {
            int seed = lane * 11 + k;
            if (hash01(seed + 600) < 0.5f) continue;
            float x = 3.0f + hash01(seed + 700) * 17.0f;
            float y = y0 + 0.25f + hash01(seed + 800) * (LANE_HEIGHT - 0.5f);
            solid.polyline({Vec2(x, y), Vec2(x + 0.26f, y + 0.16f),
                            Vec2(x + 0.14f, y + 0.34f), Vec2(x + 0.44f, y + 0.46f)},
                           0.030f, withAlpha(OUTLINE_DARK, 0.35f));
        }

        // the groove between two slabs, with a lit lower lip
        if (lane > 0) {
            solid.quad(0.0f, y0 - 0.06f, PLAYFIELD_WIDTH, 0.08f,
                       withAlpha(OUTLINE_DARK, 0.60f), withAlpha(OUTLINE_DARK, 0.60f));
            solid.quad(0.0f, y0 + 0.02f, PLAYFIELD_WIDTH, 0.05f,
                       withAlpha(STONE_LIGHT, 0.40f));
        }
    }

    // the wall's shadow along the top, a dark lip along the front edge
    solid.quad(0.0f, PLAYFIELD_HEIGHT - 0.35f, PLAYFIELD_WIDTH, 0.35f,
               withAlpha(OUTLINE_DARK, 0.45f), withAlpha(OUTLINE_DARK, 0.0f));
    solid.quad(0.0f, 0.0f, PLAYFIELD_WIDTH, 0.14f,
               withAlpha(OUTLINE_DARK, 0.0f), withAlpha(OUTLINE_DARK, 0.40f));
}

void drawLaneHighlight(const Game& game, Canvas& solid, float time)
{
    const Playfield& pf = game.playfield();
    const float x = HOME_BASE_X;              // the highlight spans the play area
    const float w = PLAYFIELD_WIDTH - 4.0f;

    // selected lanes: a lapis wash with a gold edge - this is the lane a
    // jump / duck and the next placement apply to
    for (int lane = 0; lane < pf.laneCount(); ++lane) {
        if (!pf.isLaneSelected(lane)) continue;
        float y = pf.laneBottomY(lane);
        solid.quad(x, y, w, LANE_HEIGHT, withAlpha(LAPIS, 0.26f));
        solid.quad(x, y, w, 0.06f, withAlpha(GOLD, 0.85f));
        solid.quad(x, y + LANE_HEIGHT - 0.06f, w, 0.06f, withAlpha(GOLD, 0.85f));
    }

    // hovered lane: warm light that breathes while the cursor is on it
    int hover = pf.hoveredLane();
    if (hover >= 0) {
        float y = pf.laneBottomY(hover);
        float pulse = 0.5f + 0.5f * std::sin(time * 4.0f);
        solid.quad(x, y, w, LANE_HEIGHT, withAlpha(color(0xFFD98A), 0.14f + 0.05f * pulse));
        solid.quad(x, y, w, 0.05f, withAlpha(color(0xFFE9A8), 0.55f + 0.25f * pulse));
        solid.quad(x, y + LANE_HEIGHT - 0.05f, w, 0.05f,
                   withAlpha(color(0xFFE9A8), 0.55f + 0.25f * pulse));
    }
}

void drawSunwellGate(Canvas& solid, Canvas& light, float time)
{
    const float right = HOME_BASE_X;          // 2.0 - enemies must not cross it
    float pulse = 0.5f + 0.5f * std::sin(time * 1.6f);

    // shadow the gate throws on the floor
    solid.quad(0.0f, 0.0f, right + 0.20f, 0.55f,
               withAlpha(OUTLINE_DARK, 0.0f), withAlpha(OUTLINE_DARK, 0.45f));

    // the doorway: deep and dark, the Sunwell burning inside it
    solid.quad(0.66f, 0.45f, 0.68f, 5.05f, color(0x1A1520), color(0x2E2118));

    // the well itself: halo, rays, disc
    light.glow(1.0f, 3.10f, 1.55f + 0.12f * pulse,
               withAlpha(SUN_EDGE, 0.50f + 0.12f * pulse), 6);
    for (int i = 0; i < 8; ++i) {
        float a = 0.7853982f * i + time * 0.15f;
        float inner = 0.54f;
        float outer = 0.74f + 0.06f * pulse;
        solid.line(Vec2(1.0f + inner * std::cos(a), 3.10f + inner * std::sin(a)),
                   Vec2(1.0f + outer * std::cos(a), 3.10f + outer * std::sin(a)),
                   0.05f, withAlpha(SUN_EDGE, 0.90f));
    }
    solid.circle(1.0f, 3.10f, 0.44f, SUN_EDGE, 32);
    solid.circle(1.0f, 3.10f, 0.32f, SUN_CORE, 32);
    solid.ring(1.0f, 3.10f, 0.44f, 0.50f, GOLD, 32);

    // left pylon: battered (wider at the foot), lit edge, recessed panel
    solid.fillAndOutline({Vec2(0.00f, 0.45f), Vec2(0.66f, 0.45f),
                          Vec2(0.66f, 5.50f), Vec2(0.10f, 5.50f)},
                         mix(STONE, SAND_LIGHT, 0.30f), OUTLINE_PLAYER, 0.05f);
    solid.quad(0.05f, 0.55f, 0.06f, 4.85f, withAlpha(STONE_LIGHT, 0.75f),
               withAlpha(STONE_LIGHT, 0.35f));
    solid.quad(0.17f, 1.10f, 0.34f, 3.60f, mix(STONE, STONE_SHADE, 0.55f),
               mix(STONE, STONE_SHADE, 0.35f));
    for (int i = 0; i < 3; ++i) glyph(solid, i, 0.34f, 1.75f + 1.05f * i, 0.17f, STONE_SHADE);
    solid.quad(0.10f, 4.85f, 0.56f, 0.16f, GOLD, BRONZE_DARK);

    // right pylon, mirrored
    solid.fillAndOutline({Vec2(1.34f, 0.45f), Vec2(2.00f, 0.45f),
                          Vec2(1.90f, 5.50f), Vec2(1.34f, 5.50f)},
                         mix(STONE, SAND_LIGHT, 0.30f), OUTLINE_PLAYER, 0.05f);
    solid.quad(1.89f, 0.55f, 0.06f, 4.85f, withAlpha(STONE_LIGHT, 0.75f),
               withAlpha(STONE_LIGHT, 0.35f));
    solid.quad(1.49f, 1.10f, 0.34f, 3.60f, mix(STONE, STONE_SHADE, 0.55f),
               mix(STONE, STONE_SHADE, 0.35f));
    for (int i = 0; i < 3; ++i) glyph(solid, i + 1, 1.66f, 1.75f + 1.05f * i, 0.17f, STONE_SHADE);
    solid.quad(1.34f, 4.85f, 0.56f, 0.16f, GOLD, BRONZE_DARK);

    // lintel with a winged sun, then the curved cap
    solid.fillAndOutline({Vec2(0.00f, 5.50f), Vec2(2.00f, 5.50f),
                          Vec2(2.00f, 6.40f), Vec2(0.00f, 6.40f)},
                         mix(STONE, SAND_LIGHT, 0.20f), OUTLINE_PLAYER, 0.05f);
    solid.quad(0.00f, 6.05f, 2.00f, 0.10f, GOLD, BRONZE_DARK);
    solid.circle(1.00f, 5.85f, 0.17f, GOLD, 20);
    solid.poly({Vec2(0.90f, 5.85f), Vec2(0.40f, 5.72f), Vec2(0.24f, 5.86f),
                Vec2(0.44f, 5.96f), Vec2(0.90f, 5.97f)}, GOLD);
    solid.poly({Vec2(1.10f, 5.85f), Vec2(1.60f, 5.72f), Vec2(1.76f, 5.86f),
                Vec2(1.56f, 5.96f), Vec2(1.10f, 5.97f)}, GOLD);
    solid.roundedRect(-0.04f, 6.40f, 2.08f, 0.34f, 0.14f, mix(STONE, SAND_LIGHT, 0.35f));

    // steps at the foot
    solid.quad(0.00f, 0.28f, 2.00f, 0.17f, STONE_LIGHT, STONE);
    solid.quad(0.00f, 0.00f, 2.00f, 0.28f, STONE, STONE_SHADE);

    // the threshold itself: the line an enemy must not cross (section 3.4)
    solid.quad(right - 0.03f, 0.0f, 0.06f, PLAYFIELD_HEIGHT,
               withAlpha(GOLD, 0.55f), withAlpha(GOLD, 0.55f));
    solid.quad(right - 0.10f, 0.0f, 0.30f, PLAYFIELD_HEIGHT,
               withAlpha(GOLD, 0.12f), withAlpha(GOLD, 0.12f));
}

void drawBreach(Canvas& solid, float time)
{
    const float x0 = 22.6f;

    // the shadow of the collapsed corner, stronger towards the right
    for (int i = 0; i < 5; ++i) {
        float w = (PLAYFIELD_WIDTH - x0) / 5.0f;
        solid.quad(x0 + i * w, 0.0f, w, PLAYFIELD_HEIGHT,
                   withAlpha(OUTLINE_DARK, 0.04f + 0.05f * i),
                   withAlpha(OUTLINE_DARK, 0.08f + 0.07f * i));
    }

    // the hole itself: a jagged opening, ENEMY_SPAWN_X (23.5) sits inside it
    solid.poly({Vec2(23.30f, 0.0f), Vec2(23.06f, 0.9f), Vec2(23.34f, 1.8f),
                Vec2(23.02f, 2.9f), Vec2(23.32f, 3.9f), Vec2(23.08f, 5.0f),
                Vec2(23.36f, 6.0f), Vec2(PLAYFIELD_WIDTH, 6.0f),
                Vec2(PLAYFIELD_WIDTH, 0.0f)}, color(0x120F14));

    // broken masonry left standing along the ragged edge
    for (int i = 0; i < 7; ++i) {
        float y = 0.40f + 0.85f * i;
        float x = 23.02f + 0.26f * hash01(i + 900);
        float w = 0.28f + 0.20f * hash01(i + 910);
        float h = 0.26f + 0.18f * hash01(i + 920);
        solid.quad(x - w, y, w, h,
                   mix(WALL, WALL_LIGHT, 0.40f), mix(WALL, WALL_SHADOW, 0.50f));
    }

    // a snapped column leaning in the opening
    solid.push(Transform2D::translate(23.62f, 0.0f));
    solid.push(Transform2D::rotate(0.16f));
    solid.quad(-0.24f, 0.0f, 0.48f, 3.40f,
               mix(WALL, WALL_LIGHT, 0.35f), mix(WALL, WALL_SHADOW, 0.45f));
    for (int i = 1; i < 5; ++i)
        solid.quad(-0.24f, 0.70f * i, 0.48f, 0.06f, withAlpha(WALL_SHADOW, 0.60f));
    solid.poly({Vec2(-0.24f, 3.40f), Vec2(-0.05f, 3.78f), Vec2(0.14f, 3.44f),
                Vec2(0.24f, 3.64f), Vec2(0.24f, 3.40f)},
               mix(WALL, WALL_SHADOW, 0.30f));
    solid.pop();
    solid.pop();

    // rubble pile at the foot of the hole
    for (int i = 0; i < 5; ++i) {
        float x = 23.30f + 0.65f * hash01(i + 930);
        float r = 0.16f + 0.14f * hash01(i + 940);
        solid.ellipse(x, r * 0.55f, r, r * 0.75f, 0.0f,
                      mix(WALL, WALL_SHADOW, 0.35f + 0.3f * hash01(i + 945)), 12);
    }

    // sand trickling down from the broken edge
    for (int i = 0; i < 6; ++i) {
        float x = 23.15f + 0.55f * hash01(i + 950);
        float t = std::fmod(time * (0.45f + 0.30f * hash01(i + 960)) + hash01(i + 970), 1.0f);
        float y = 6.0f - t * 5.6f;
        solid.circle(x, y, 0.035f, withAlpha(SAND_LIGHT, 0.55f * (1.0f - t)), 6);
    }
}

void drawProps(Canvas& solid, Canvas& light, float time)
{
    // braziers at the foot of the wall, one in every other wall bay
    brazier(solid, light, 5.42f, PLAYFIELD_HEIGHT + 0.05f, time, 0.0f);
    brazier(solid, light, 11.42f, PLAYFIELD_HEIGHT + 0.05f, time, 2.1f);
    brazier(solid, light, 17.42f, PLAYFIELD_HEIGHT + 0.05f, time, 4.4f);

    // urns beside the gate (players are placed from x = 2.5, so nothing here)
    urn(solid, 2.22f, 0.06f, 0.95f);
    urn(solid, 2.18f, 2 * LANE_HEIGHT + 0.06f, 0.80f);

    // bones and shards where the desecrators pile in, and a little at the gate
    for (int lane = 0; lane < LANE_COUNT; ++lane)
        debris(solid, 22.85f, lane * LANE_HEIGHT + 0.05f, 40 + lane * 7);
    debris(solid, 3.10f, 3 * LANE_HEIGHT + 0.05f, 91);
}

} // namespace

void drawCutsceneEnvironment(Canvas& solid, Canvas& light,
                             float time, float viewHeight)
{
    const Color skyTop(0.12f, 0.29f, 0.34f);
    const Color skyBottom(0.04f, 0.13f, 0.14f);
    solid.quad(0.0f, 0.0f, PLAYFIELD_WIDTH, viewHeight, skyTop, skyBottom);

    const float moonY = viewHeight - 1.7f;
    solid.circle(20.3f, moonY, 0.55f, Color(0.95f, 0.82f, 0.55f), 32);
    light.glow(20.3f, moonY, 1.8f, Color(0.95f, 0.70f, 0.36f, 0.28f));

    solid.poly({Vec2(0.0f, 6.0f), Vec2(2.4f, 7.3f), Vec2(5.5f, 6.1f),
                Vec2(8.8f, 7.5f), Vec2(12.1f, 6.1f), Vec2(16.0f, 7.2f),
                Vec2(19.2f, 6.1f), Vec2(22.0f, 7.4f), Vec2(24.0f, 6.2f),
                Vec2(24.0f, 0.0f), Vec2(0.0f, 0.0f)}, Color(0.12f, 0.27f, 0.18f));

    solid.quad(0.0f, 0.0f, PLAYFIELD_WIDTH, PLAYFIELD_HEIGHT,
               Color(0.22f, 0.36f, 0.19f), Color(0.08f, 0.19f, 0.12f));
    for (int lane = 0; lane < LANE_COUNT; ++lane) {
        const float y = lane * LANE_HEIGHT;
        solid.quad(0.0f, y + 0.12f, PLAYFIELD_WIDTH, LANE_HEIGHT - 0.24f,
                   Color(0.35f, 0.39f, 0.19f, 0.34f));
        solid.quad(0.0f, y + 0.10f, PLAYFIELD_WIDTH, 0.035f,
                   Color(0.73f, 0.68f, 0.37f, 0.50f));
    }

    solid.roundedRect(9.7f, 4.35f, 4.6f, 2.0f, 0.22f,
                      Color(0.38f, 0.31f, 0.20f));
    solid.poly({Vec2(9.3f, 6.25f), Vec2(12.0f, 7.25f), Vec2(14.7f, 6.25f)},
               Color(0.48f, 0.37f, 0.20f));
    solid.roundedRect(11.0f, 4.35f, 2.0f, 1.45f, 0.32f,
                      Color(0.12f, 0.20f, 0.14f));
    solid.circle(12.0f, 5.55f, 0.25f, Color(0.94f, 0.76f, 0.35f), 20);
    solid.ellipse(12.0f, 5.55f, 0.10f, 0.21f, -0.35f,
                  Color(0.48f, 0.72f, 0.31f), 16);
    light.glow(12.0f, 5.55f, 0.9f, Color(0.92f, 0.72f, 0.30f, 0.22f));

    for (int i = 0; i < 6; ++i) {
        const float x = 0.8f + i * 4.5f;
        if (x > 9.0f && x < 15.0f) continue;
        const float sway = std::sin(time * 2.0f + i) * 0.06f;
        solid.line(Vec2(x, 0.15f), Vec2(x + sway, 0.75f), 0.10f,
                   Color(0.19f, 0.48f, 0.20f));
        solid.ellipse(x - 0.17f, 0.52f, 0.25f, 0.12f, -0.35f,
                      Color(0.37f, 0.67f, 0.26f), 16);
        solid.ellipse(x + 0.17f, 0.66f, 0.25f, 0.12f, 0.35f,
                      Color(0.43f, 0.72f, 0.29f), 16);
    }
}

void drawEnvironment(const Game& game, Canvas& solid, Canvas& light,
                     float time, float viewHeight)
{
    drawSky(solid, light, time, viewHeight);
    drawDistantScenery(solid);
    drawTempleWall(solid);
    drawGround(solid);
    drawLaneHighlight(game, solid, time);
    drawSunwellGate(solid, light, time);
    drawBreach(solid, time);
    drawProps(solid, light, time);
}
