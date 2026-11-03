// ---------------------------------------------------------------------------
// Preview/preview.cpp  -  development tool, NOT part of the submission.
//
// Renders into a HIDDEN window and saves the framebuffer as a .ppm picture, so
// the artwork can be inspected without a screen capture (which this machine
// does not allow). PPM is a three-line format, no library needed:
//
//     python3 -c "from PIL import Image; Image.open('a.ppm').save('a.png')"
//
// The sheet content is drawn in WORLD units (0..24, 0..6 - exactly the space
// the game logic uses), so the same content under the different modes shows
// how each projection treats a unit:
//
//   ./preview sheet out.ppm game    = main.cpp: view height follows the window
//                                     aspect -> square units, sky band above
//   ./preview sheet out.ppm old     = what main.cpp did before: 24x6 stretched
//                                     into a 16:9 window (y is 2.25x)
//   ./preview sheet out.ppm square  = 4:1 window with a 24x6 view
//                                     = the plain reference
//
// build (macOS, GLFW built from source - the vendored glfw3.lib is Windows):
//   g++ -std=c++17 -I. -IExternal -IExternal/include Preview/preview.cpp \
//       Render/Canvas.cpp Render/Renderer.cpp Render/SceneRenderer.cpp \
//       Playfield.cpp Player.cpp Enemy.cpp Projectile.cpp Collectibles.cpp \
//       Economy.cpp Waves.cpp Menus.cpp Cutscene.cpp Game.cpp \
//       glad.o -L<glfw>/src -lglfw3 -framework Cocoa -framework IOKit \
//       -framework CoreVideo -framework OpenGL -o preview
// ---------------------------------------------------------------------------

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Render/Canvas.h"
#include "Render/Palette.h"
#include "Render/Renderer.h"

namespace {

int g_fbWidth  = 2560;
int g_fbHeight = 1440;

GLFWwindow* createHiddenWindow(int width, int height)
{
    if (!glfwInit()) return nullptr;

    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
#ifdef __APPLE__
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#else
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
#endif
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(width, height, "preview", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return nullptr;
    }
    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        glfwDestroyWindow(window);
        glfwTerminate();
        return nullptr;
    }

    glfwGetFramebufferSize(window, &g_fbWidth, &g_fbHeight);
    glViewport(0, 0, g_fbWidth, g_fbHeight);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    return window;
}

void savePpm(const char* path)
{
    std::vector<unsigned char> pixels(g_fbWidth * g_fbHeight * 3);
    glReadPixels(0, 0, g_fbWidth, g_fbHeight, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

    FILE* file = std::fopen(path, "wb");
    if (!file) {
        std::printf("cannot write %s\n", path);
        return;
    }
    std::fprintf(file, "P6\n%d %d\n255\n", g_fbWidth, g_fbHeight);

    // glReadPixels starts at the bottom row, a PPM starts at the top
    for (int y = g_fbHeight - 1; y >= 0; --y) {
        const unsigned char* row = &pixels[static_cast<size_t>(y) * g_fbWidth * 3];
        std::fwrite(row, 1, static_cast<size_t>(g_fbWidth) * 3, file);
    }
    std::fclose(file);
    std::printf("wrote %s (%dx%d)\n", path, g_fbWidth, g_fbHeight);
}

// Polygon of the region inside the circle (R at cx,cy) and outside the circle
// (r, whose centre sits `off` to the right). The arcs must start and end where
// the two circles actually cross, otherwise the polygon closes through itself.
std::vector<Vec2> crescentPoints(float cx, float cy, float R, float r, float off)
{
    float a = (off * off + R * R - r * r) / (2.0f * off);   // centre of the chord
    float h = std::sqrt(R * R - a * a);                     // half of its length

    float outerFrom = std::atan2(h, a);              // upper crossing, seen from A
    float outerTo   = 6.2831853f - outerFrom;        // lower crossing
    float innerFrom = 6.2831853f - std::atan2(h, a - off);   // lower, from B
    float innerTo   = std::atan2(h, a - off);               // upper, from B

    std::vector<Vec2> pts;
    for (int i = 0; i <= 24; ++i) {                  // outer edge, counter-clockwise
        float t = outerFrom + (outerTo - outerFrom) * static_cast<float>(i) / 24.0f;
        pts.push_back(Vec2(cx + R * std::cos(t), cy + R * std::sin(t)));
    }
    for (int i = 0; i <= 24; ++i) {                  // inner edge, back clockwise
        float t = innerFrom + (innerTo - innerFrom) * static_cast<float>(i) / 24.0f;
        pts.push_back(Vec2(cx + off + r * std::cos(t), cy + r * std::sin(t)));
    }
    return pts;
}

// A small figure assembled from several shapes: proves the transform stack and
// shows the level of detail the character art is expected to reach.
void drawSampleFigure(Canvas& c)
{
    c.push(Transform2D::translate(12.6f, 1.15f));

    c.quad(-0.30f, -0.60f, 0.60f, 0.75f, lighten(LINEN, 0.15f), LINEN_SHADE);
    c.fillAndOutline({Vec2(-0.34f, -0.62f), Vec2(0.34f, -0.62f),
                      Vec2(0.26f, 0.16f), Vec2(-0.26f, 0.16f)},
                     BRONZE, OUTLINE_PLAYER, 0.03f);
    c.circle(0.0f, 0.34f, 0.20f, color(0xC9905A), 20);
    c.fillAndOutline({Vec2(-0.26f, 0.34f), Vec2(0.26f, 0.34f),
                      Vec2(0.18f, 0.62f), Vec2(-0.18f, 0.62f)},
                     LAPIS, OUTLINE_PLAYER, 0.03f);

    // an arm holding a weapon, rotated around the shoulder
    c.push(Transform2D::translate(0.24f, 0.08f));
    c.push(Transform2D::rotate(0.5f));
    c.quad(0.0f, -0.08f, 0.42f, 0.16f, lighten(BRONZE, 0.1f), BRONZE_DARK);
    c.push(Transform2D::translate(0.44f, 0.0f));
    c.fillAndOutline({Vec2(-0.05f, -0.07f), Vec2(0.46f, -0.13f),
                      Vec2(0.62f, 0.02f), Vec2(0.40f, 0.14f),
                      Vec2(-0.05f, 0.07f)},
                     BRONZE_LIGHT, OUTLINE_PLAYER, 0.03f);
    c.pop();
    c.pop();

    c.glow(0.0f, 0.34f, 0.5f, withAlpha(GOLD, 0.35f));
    c.pop();
}

// Everything below is drawn in world units: x in [0, 24], y in [0, 6].
// With a taller view (the "game" mode) the band above the playfield is filled
// with a plain dusk gradient, so the frame can be judged as a whole.
void drawTestSheet(Canvas& solid, Canvas& light, float viewHeight)
{
    if (viewHeight > PLAYFIELD_HEIGHT + 0.1f) {
        solid.quad(0.0f, PLAYFIELD_HEIGHT, PLAYFIELD_WIDTH,
                   viewHeight - PLAYFIELD_HEIGHT,
                   color(0x1B2340), color(0x4A3352));
    }
    solid.quad(0.0f, 0.0f, PLAYFIELD_WIDTH, PLAYFIELD_HEIGHT, color(0x101426));

    // --- row 1 (top): gradients, roundness, rounded corners ----------------
    solid.quad(0.6f, 4.4f, 3.6f, 1.2f, color(0xF0C27B), color(0x4B1213));

    solid.push(Transform2D::translate(6.1f, 5.0f));
    solid.circle(0.0f, 0.0f, 0.62f, color(0x6FA8D6), 40);
    solid.ring(0.0f, 0.0f, 0.30f, 0.42f, color(0xE9B949), 40);
    solid.pop();

    solid.roundedRect(8.2f, 4.4f, 3.0f, 1.2f, 0.35f, color(0x8E6BB5));
    solid.fillAndOutline({Vec2(12.6f, 4.4f), Vec2(15.6f, 4.4f),
                          Vec2(14.8f, 5.6f), Vec2(13.4f, 5.6f)},
                         color(0x3F7D5A), OUTLINE_PLAYER, 0.04f);

    // crescent: a concave polygon, the case ear clipping exists for
    std::vector<Vec2> crescent = crescentPoints(17.6f, 5.0f, 0.72f, 0.57f, 0.35f);
    solid.fillAndOutline(crescent, color(0xE8B33C), OUTLINE_PLAYER, 0.035f);

    solid.push(Transform2D::translate(21.6f, 5.0f));
    solid.push(Transform2D::rotate(-0.5f));
    solid.ellipse(0.0f, 0.0f, 1.0f, 0.5f, 0.0f, color(0x4FA3A0), 32);
    solid.pop();
    solid.pop();

    // --- row 2 (middle): strokes, curves, sectors --------------------------
    solid.line(Vec2(0.7f, 2.7f), Vec2(4.0f, 3.9f), 0.22f, color(0xB4643C));
    solid.line(Vec2(0.7f, 3.9f), Vec2(4.0f, 2.7f), 0.06f, color(0x5F7A3E));

    solid.bezier(Vec2(5.4f, 2.7f), Vec2(6.6f, 4.2f),
                 Vec2(8.4f, 4.2f), Vec2(9.6f, 2.7f), 0.10f, EBONY);
    solid.line(Vec2(5.4f, 2.7f), Vec2(9.6f, 2.7f), 0.03f, IVORY);

    solid.sector(12.2f, 3.2f, 0.95f, 0.35f, 2.5f, color(0xC03028), 20);
    solid.ring(12.2f, 3.2f, 1.02f, 1.12f, color(0xE9B949), 32);

    solid.bezier(Vec2(14.6f, 2.7f), Vec2(15.4f, 4.0f),
                 Vec2(17.4f, 2.4f), Vec2(18.4f, 3.9f), 0.07f, color(0x6FA8D6));

    solid.push(Transform2D::translate(21.2f, 3.3f));
    solid.fillAndOutline({Vec2(-0.9f, -0.5f), Vec2(0.9f, -0.35f),
                          Vec2(0.7f, 0.5f), Vec2(-0.8f, 0.4f)},
                         color(0x8E6BB5), OUTLINE_ENEMY, 0.03f);
    solid.pop();

    // --- row 3 (bottom): outlines and thickness, transform figure ----------
    solid.fillAndOutline({Vec2(0.6f, 0.5f), Vec2(4.4f, 0.5f),
                          Vec2(5.4f, 2.1f), Vec2(1.6f, 2.3f)},
                         color(0x2E5FA3), OUTLINE_PLAYER, 0.025f);
    solid.fillAndOutline({Vec2(6.6f, 0.5f), Vec2(10.0f, 0.8f),
                          Vec2(10.5f, 2.2f), Vec2(7.1f, 2.05f)},
                         color(0x5F7A3E), OUTLINE_ENEMY, 0.05f);
    drawSampleFigure(solid);

    // --- additive light ----------------------------------------------------
    light.circle(16.2f, 1.2f, 0.45f, color(0xC03028, 0.75f), 24);
    light.glow(18.6f, 1.3f, 1.2f, color(0xFFB347, 0.85f));
    light.quad(20.6f, 1.1f, 2.2f, 0.16f, color(0xFFE9A8, 0.8f));
    light.glow(18.6f, 4.2f, 1.1f, color(0x7FC7FF, 0.6f));
}

} // namespace

int main(int argc, char** argv)
{
    const char* mode    = (argc > 1) ? argv[1] : "sheet";
    const char* outPath = (argc > 2) ? argv[2] : "preview.ppm";
    const char* space   = (argc > 3) ? argv[3] : "game";

    if (std::strcmp(mode, "sheet") != 0) {
        std::printf("unknown mode '%s' (only 'sheet' so far)\n", mode);
        return 1;
    }

    // window: 16:9 like the game, 4:1 for the plain reference
    int winW = 1280;
    int winH = (std::strcmp(space, "square") == 0) ? 320 : 720;

    GLFWwindow* window = createHiddenWindow(winW, winH);
    if (!window) {
        std::printf("could not create an OpenGL window\n");
        return 1;
    }

    Renderer renderer;
    if (!renderer.init()) return 1;

    float aspect = static_cast<float>(g_fbWidth) / static_cast<float>(g_fbHeight);

    // "game"   = main.cpp today: view height follows the window aspect, so a
    //            unit is square and the band above the playfield is the sky
    // "old"    = what main.cpp did before: 24x6 squeezed into a 16:9 window
    // "square" = 4:1 window around the same 24x6 box = the plain reference
    float viewHeight = PLAYFIELD_HEIGHT;
    if (std::strcmp(space, "game") == 0) viewHeight = PLAYFIELD_WIDTH / aspect;

    glm::mat4 projection = glm::ortho(0.0f, PLAYFIELD_WIDTH, 0.0f, viewHeight, -1.0f, 1.0f);
    renderer.setProjection(glm::value_ptr(projection));

    Canvas solid;
    Canvas light;
    drawTestSheet(solid, light, viewHeight);

    glClearColor(0.02f, 0.02f, 0.04f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    renderer.draw(solid.vertices(), BlendMode::Alpha);
    renderer.draw(light.vertices(), BlendMode::Additive);

    savePpm(outPath);

    renderer.shutdown();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
