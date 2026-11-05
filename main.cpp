// ---------------------------------------------------------------------------
// main.cpp
//
// The OpenGL host turns GLFW input into InputState, updates the game, and draws
// the garden, interface, menus, and cutscene with the existing shader.
//
// The renderer reads the Game's read-only accessors and leaves game rules in
// the logic layer.
// ---------------------------------------------------------------------------

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <array>
#include <cctype>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Game.h"

// --- PART 3 (visuals) ------------------------------------------------------
// Everything between the PART 3 markers belongs to the rendering layer of the
// project. Part 1 owns the architecture around it; see PART3_RENDERING.md.
#include "Render/SceneRenderer.h"
// ---------------------------------------------------------------------------

int g_screenWidth = 1280;
int g_screenHeight = 720;

Game       g_game;
InputState g_input;

// --- PART 3 (visuals) ------------------------------------------------------
SceneRenderer g_sceneRenderer;
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// shaders (identical style to Lab2 Template / Lab4Camera)
// ---------------------------------------------------------------------------

// --- PART 3 (platform guard) ----------------------------------------------
// The GLSL version is the only per-platform difference: macOS stops at
// OpenGL 4.1, Windows keeps the 4.3 core profile of the labs. The bodies of
// both shaders are untouched.
#ifdef __APPLE__
#define GLSL_VERSION "#version 410 core\n"
#else
#define GLSL_VERSION "#version 430 core\n"
#endif
// ---------------------------------------------------------------------------

const char* vertexShaderSource = GLSL_VERSION R"(

layout (location = 0) in vec3 aPos;     
layout (location = 1) in vec3 aColor;   
layout (location = 2) in float aAlpha;

out vec3 vColor;                        
out float vAlpha;

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Projection;

void main()
{
    gl_Position = u_Projection * u_View * u_Model * vec4(aPos, 1.0);

    vColor = aColor;
    vAlpha = aAlpha;
}
)";

const char* fragmentShaderSource = GLSL_VERSION R"(

in vec3 vColor;
in float vAlpha;
out vec4 FragColor;

void main()
{
    FragColor = vec4(vColor, vAlpha);
}
)";

// ---------------------------------------------------------------------------
// screen <-> logic space
//
// The gameplay lives in PLAYFIELD_WIDTH x PLAYFIELD_HEIGHT logic units with y
// pointing UP. The window is 16:9 while the playfield is 4:1, so the view is
// made exactly as tall as the window aspect instead of stretching 6 units over
// the whole height: one unit stays square on screen, the playfield keeps its
// y in [0, PLAYFIELD_HEIGHT], and the band above it is free for sky and HUD
// (see PART3_RENDERING.md).
//
// Cursor positions and glfwGetWindowSize are both given in screen points, so
// the mouse mapping uses the window size - the framebuffer callbacks deliver
// pixels, which is twice as much on a Retina display.
// ---------------------------------------------------------------------------

float viewHeight()
{
    return PLAYFIELD_WIDTH * static_cast<float>(g_screenHeight) /
           static_cast<float>(g_screenWidth);
}

float screenToLogicX(double xpos, int windowWidth)
{
    if (windowWidth <= 0) return 0.0f;
    return static_cast<float>(xpos / windowWidth) * PLAYFIELD_WIDTH;
}

float screenToLogicY(double ypos, int windowHeight)
{
    if (windowHeight <= 0) return 0.0f;
    return (1.0f - static_cast<float>(ypos / windowHeight)) * viewHeight();
}

// Same idea as the ortho camera of Lab4: x grows to the right, y grows up, and
// one logic unit is one unit on screen, so the renderer can pass the positions
// of the logic straight to the shader.
glm::mat4 makeProjection()
{
    return glm::ortho(0.0f, PLAYFIELD_WIDTH, 0.0f, viewHeight(), -1.0f, 1.0f);
}

void framebufferSizeCallback(GLFWwindow* /*window*/, int width, int height)
{
    g_screenWidth = width;
    g_screenHeight = height;
    glViewport(0, 0, width, height);
}

void cursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
    int windowWidth = 0;
    int windowHeight = 0;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);

    g_input.mouseX = screenToLogicX(xpos, windowWidth);
    g_input.mouseY = screenToLogicY(ypos, windowHeight);
    g_input.mouseMoved = true;
}

// A click is an EDGE: it is true for exactly the frame it happened in.
void mouseButtonCallback(GLFWwindow* /*window*/, int button, int action, int /*mods*/)
{
    if (button != GLFW_MOUSE_BUTTON_LEFT) return;
    if (action == GLFW_PRESS) g_input.leftClickPressed = true;
}

void keyCallback(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/)
{
    if (action != GLFW_PRESS) return;      // edges only, GLFW_REPEAT is ignored

    switch (key) {
    case GLFW_KEY_ESCAPE:  g_input.escapePressed = true; break;
    case GLFW_KEY_ENTER:   g_input.enterPressed = true;  break;

    case GLFW_KEY_W:       g_input.wPressed = true;      break;
    case GLFW_KEY_S:       g_input.sPressed = true;      break;
    case GLFW_KEY_UP:      g_input.upPressed = true;     break;
    case GLFW_KEY_DOWN:    g_input.downPressed = true;   break;

    // the player cards (a real UI will call g_game.selectCard() on click)
    case GLFW_KEY_1:       g_input.card1Pressed = true;  break;
    case GLFW_KEY_2:       g_input.card2Pressed = true;  break;
    case GLFW_KEY_3:       g_input.card3Pressed = true;  break;

    case GLFW_KEY_Q:
        glfwSetWindowShouldClose(window, true);
        break;
    }
}

// ---------------------------------------------------------------------------
// frame helper: show what the logic reported
// ---------------------------------------------------------------------------

void printEvents()
{
    std::vector<GameEvent> events = g_game.drainEvents();
    for (size_t i = 0; i < events.size(); ++i) {
        std::cout << "[event] " << toString(events[i].sfx);
        if (!events[i].message.empty()) std::cout << " : " << events[i].message;
        std::cout << "\n";
    }
}

void printCutsceneLine()
{
    // the cutscene produces subtitle DATA; printing it keeps the scene
    // readable while no text renderer exists yet
    static std::string lastLine;
    const std::string& line = g_game.cutscene().currentSubtitle();
    if (line == lastLine) return;
    lastLine = line;

    if (!line.empty()) {
        std::cout << "[cutscene] " << g_game.cutscene().currentSpeaker()
                  << ": " << line << "\n";
    }
}

// ---------------------------------------------------------------------------
// gl helpers (identical to the compileShader of Lab2 / Lab4)
// ---------------------------------------------------------------------------

GLuint compileShader(GLenum type, const char* source)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Shader compile error ("
            << (type == GL_VERTEX_SHADER ? "vertex" : "fragment")
            << "):\n" << infoLog << std::endl;
    }
    return shader;
}

// The UI uses a tiny built-in bitmap font and batched coloured quads, so it
// remains readable without external art or font dependencies.
std::vector<float> g_uiVertices;

void addRect(float x, float y, float width, float height,
             float red, float green, float blue, float alpha = 1.0f)
{
    const float vertices[] = {
        x, y, 0.0f, red, green, blue, alpha,
        x + width, y, 0.0f, red, green, blue, alpha,
        x + width, y + height, 0.0f, red, green, blue, alpha,
        x, y, 0.0f, red, green, blue, alpha,
        x + width, y + height, 0.0f, red, green, blue, alpha,
        x, y + height, 0.0f, red, green, blue, alpha
    };
    g_uiVertices.insert(g_uiVertices.end(), vertices, vertices + 42);
}

std::array<unsigned char, 7> glyph(char value)
{
    switch (value) {
    case 'A': return {14, 17, 17, 31, 17, 17, 17};
    case 'B': return {30, 17, 17, 30, 17, 17, 30};
    case 'C': return {14, 17, 16, 16, 16, 17, 14};
    case 'D': return {30, 17, 17, 17, 17, 17, 30};
    case 'E': return {31, 16, 16, 30, 16, 16, 31};
    case 'F': return {31, 16, 16, 30, 16, 16, 16};
    case 'G': return {14, 17, 16, 23, 17, 17, 15};
    case 'H': return {17, 17, 17, 31, 17, 17, 17};
    case 'I': return {14, 4, 4, 4, 4, 4, 14};
    case 'J': return {7, 2, 2, 2, 18, 18, 12};
    case 'K': return {17, 18, 20, 24, 20, 18, 17};
    case 'L': return {16, 16, 16, 16, 16, 16, 31};
    case 'M': return {17, 27, 21, 21, 17, 17, 17};
    case 'N': return {17, 25, 21, 19, 17, 17, 17};
    case 'O': return {14, 17, 17, 17, 17, 17, 14};
    case 'P': return {30, 17, 17, 30, 16, 16, 16};
    case 'Q': return {14, 17, 17, 17, 21, 18, 13};
    case 'R': return {30, 17, 17, 30, 20, 18, 17};
    case 'S': return {15, 16, 16, 14, 1, 1, 30};
    case 'T': return {31, 4, 4, 4, 4, 4, 4};
    case 'U': return {17, 17, 17, 17, 17, 17, 14};
    case 'V': return {17, 17, 17, 17, 17, 10, 4};
    case 'W': return {17, 17, 17, 21, 21, 21, 10};
    case 'X': return {17, 17, 10, 4, 10, 17, 17};
    case 'Y': return {17, 17, 10, 4, 4, 4, 4};
    case 'Z': return {31, 1, 2, 4, 8, 16, 31};
    case '0': return {14, 17, 19, 21, 25, 17, 14};
    case '1': return {4, 12, 4, 4, 4, 4, 14};
    case '2': return {14, 17, 1, 2, 4, 8, 31};
    case '3': return {30, 1, 1, 14, 1, 1, 30};
    case '4': return {2, 6, 10, 18, 31, 2, 2};
    case '5': return {31, 16, 16, 30, 1, 1, 30};
    case '6': return {14, 16, 16, 30, 17, 17, 14};
    case '7': return {31, 1, 2, 4, 8, 8, 8};
    case '8': return {14, 17, 17, 14, 17, 17, 14};
    case '9': return {14, 17, 17, 15, 1, 1, 14};
    case ':': return {0, 4, 4, 0, 4, 4, 0};
    case '.': return {0, 0, 0, 0, 0, 12, 12};
    case ',': return {0, 0, 0, 0, 4, 4, 8};
    case '!': return {4, 4, 4, 4, 4, 0, 4};
    case '?': return {14, 17, 1, 2, 4, 0, 4};
    case '-': return {0, 0, 0, 31, 0, 0, 0};
    case '+': return {0, 4, 4, 31, 4, 4, 0};
    case '/': return {1, 2, 2, 4, 8, 8, 16};
    case '>': return {16, 8, 4, 2, 4, 8, 16};
    case '\'': return {4, 4, 8, 0, 0, 0, 0};
    default: return {0, 0, 0, 0, 0, 0, 0};
    }
}

void addText(float x, float y, const std::string& text, float cellWidth,
             float red, float green, float blue)
{
    const float cellHeight = cellWidth * (static_cast<float>(g_screenWidth) /
                                           static_cast<float>(g_screenHeight)) / 4.0f;
    float cursorX = x;
    for (size_t i = 0; i < text.size(); ++i) {
        const char ch = static_cast<char>(std::toupper(
            static_cast<unsigned char>(text[i])));
        const std::array<unsigned char, 7> rows = glyph(ch);
        for (int row = 0; row < 7; ++row) {
            for (int col = 0; col < 5; ++col) {
                if ((rows[row] & (1 << (4 - col))) != 0) {
                    addRect(cursorX + col * cellWidth,
                            y + (6 - row) * cellHeight,
                            cellWidth * 0.78f, cellHeight * 0.78f,
                            red, green, blue);
                }
            }
        }
        cursorX += cellWidth * 6.0f;
    }
}

void flushShapes(GLuint vao, GLuint vbo)
{
    if (g_uiVertices.empty()) return;
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(g_uiVertices.size() * sizeof(float)),
                 &g_uiVertices[0], GL_STREAM_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(g_uiVertices.size() / 7));
    g_uiVertices.clear();
}

float menuRowY(int index)
{
    return 3.12f - static_cast<float>(index) * 0.72f;
}

int menuHitTest(float x, float y, const Menu& menu)
{
    if (x < 13.0f || x > 22.0f) return -1;
    for (int i = 0; i < menu.itemCount(); ++i) {
        const float rowY = menuRowY(i);
        if (y >= rowY - 0.12f && y <= rowY + 0.48f) return i;
    }
    return -1;
}

bool pauseHitTest(float x, float y)
{
    return x >= 18.9f && x <= 22.2f && y >= 5.68f && y <= 5.98f;
}

bool skipHitTest(float x, float y)
{
    return x >= 19.0f && x <= 22.2f && y >= 5.50f && y <= 5.90f;
}

bool handleScreenControlClick(GameState state, InputState& input)
{
    if (!input.leftClickPressed) return false;

    const bool clickedSkip = state == GameState::Cutscene &&
                             skipHitTest(input.mouseX, input.mouseY);
    const bool clickedPause = state == GameState::Playing &&
                              pauseHitTest(input.mouseX, input.mouseY);
    if (!clickedSkip && !clickedPause) return false;

    input.escapePressed = true;
    input.leftClickPressed = false;
    return true;
}

int cardHitTest(float x, float y)
{
    if (y < 0.08f || y > 0.72f) return -1;
    for (int i = 0; i < 3; ++i) {
        const float left = 7.0f + i * 3.25f;
        if (x >= left && x <= left + 2.9f) return i;
    }
    return -1;
}

void drawGardenBackdrop()
{
    addRect(0.0f, 0.0f, 24.0f, 6.0f, 0.07f, 0.14f, 0.20f);
    addRect(0.0f, 3.7f, 24.0f, 2.3f, 0.10f, 0.22f, 0.29f);
    addRect(0.0f, 0.0f, 24.0f, 1.15f, 0.12f, 0.27f, 0.18f);
    addRect(0.0f, 1.12f, 24.0f, 0.14f, 0.28f, 0.37f, 0.20f);
    for (int i = 0; i < 6; ++i) {
        const float x = 0.4f + i * 4.0f;
        addRect(x, 1.25f, 0.09f, 2.0f + (i % 3) * 0.22f,
                0.08f, 0.19f, 0.16f);
        addRect(x - 0.24f, 2.5f, 0.56f, 0.12f, 0.17f, 0.38f, 0.26f);
        addRect(x - 0.16f, 2.64f, 0.40f, 0.12f, 0.20f, 0.44f, 0.30f);
    }
    // A small leaf-and-stem emblem gives the title screen a welcoming focal point.
    addRect(4.0f, 2.0f, 0.20f, 1.15f, 0.28f, 0.63f, 0.31f);
    addRect(3.2f, 2.65f, 0.88f, 0.20f, 0.36f, 0.76f, 0.35f);
    addRect(4.2f, 2.96f, 0.88f, 0.20f, 0.36f, 0.76f, 0.35f);
    addRect(3.72f, 3.2f, 0.74f, 0.74f, 0.90f, 0.76f, 0.30f);
    addRect(3.97f, 3.43f, 0.24f, 0.24f, 0.11f, 0.21f, 0.16f);
}

void drawBattlefield(const Game& game)
{
    // The lanes, their highlights, the home base and the enemy entry are all
    // drawn by the scene renderer (Render/Environment.cpp) before this runs.
    const Playfield& field = game.playfield();

    for (int lane = 0; lane < field.laneCount(); ++lane) {
        const float cy = field.laneCenterY(lane);
        addText(0.18f, cy - 0.09f, std::to_string(lane + 1), 0.055f,
                0.84f, 0.88f, 0.68f);
    }

    for (size_t i = 0; i < game.players().size(); ++i) {
        const Player& player = game.players()[i];
        const float y = field.laneCenterY(player.lane);
        float red = 0.35f, green = 0.73f, blue = 0.32f;
        if (player.type == PlayerType::Ranged) { red = 0.34f; green = 0.69f; blue = 0.86f; }
        if (player.type == PlayerType::Defense) { red = 0.88f; green = 0.66f; blue = 0.27f; }
        if (player.hitFlash > 0.0f) { red = 0.92f; green = 0.28f; blue = 0.24f; }
        const float bob = player.state == CharState::Advancing
            ? std::sin(player.animTime * 8.0f) * 0.045f : 0.0f;
        addRect(player.x - 0.20f, y - 0.24f + bob, 0.40f, 0.38f, red, green, blue);
        addRect(player.x - 0.15f, y + 0.14f + bob, 0.30f, 0.25f,
                0.78f, 0.83f, 0.61f);
        addRect(player.x - 0.12f, y + 0.23f + bob, 0.05f, 0.05f,
                0.10f, 0.16f, 0.12f);
        addRect(player.x + 0.06f, y + 0.23f + bob, 0.05f, 0.05f,
                0.10f, 0.16f, 0.12f);
        addRect(player.x - 0.22f, y + 0.45f, 0.44f, 0.07f, 0.10f, 0.14f, 0.12f);
        addRect(player.x - 0.22f, y + 0.45f, 0.44f * player.healthRatio(), 0.07f,
                0.35f, 0.88f, 0.35f);
    }

    for (size_t i = 0; i < game.enemies().size(); ++i) {
        const Enemy& enemy = game.enemies()[i];
        const float y = field.laneCenterY(enemy.lane);
        float red = 0.72f, green = 0.32f, blue = 0.36f;
        if (enemy.type == EnemyType::MediumLower) { red = 0.68f; green = 0.40f; blue = 0.75f; }
        if (enemy.type == EnemyType::HardMelee) { red = 0.80f; green = 0.26f; blue = 0.19f; }
        if (enemy.hitFlash > 0.0f) { red = 1.0f; green = 0.72f; blue = 0.40f; }
        const float bob = enemy.state == CharState::Advancing
            ? std::sin(enemy.animTime * 7.0f) * 0.035f : 0.0f;
        addRect(enemy.x - 0.20f, y - 0.23f + bob, 0.40f, 0.39f, red, green, blue);
        addRect(enemy.x - 0.16f, y + 0.14f + bob, 0.32f, 0.25f,
                0.71f, 0.76f, 0.58f);
        addRect(enemy.x - 0.12f, y + 0.22f + bob, 0.06f, 0.06f,
                0.16f, 0.10f, 0.10f);
        addRect(enemy.x + 0.06f, y + 0.22f + bob, 0.06f, 0.06f,
                0.16f, 0.10f, 0.10f);
        addRect(enemy.x - 0.22f, y + 0.45f, 0.44f, 0.07f, 0.10f, 0.14f, 0.12f);
        addRect(enemy.x - 0.22f, y + 0.45f, 0.44f * enemy.healthRatio(), 0.07f,
                0.90f, 0.31f, 0.24f);
    }

    for (size_t i = 0; i < game.projectiles().size(); ++i) {
        const Projectile& shot = game.projectiles()[i];
        addRect(shot.x - 0.09f, shot.y - 0.035f, 0.18f, 0.07f,
                shot.faction == Faction::Player ? 0.98f : 0.94f,
                shot.faction == Faction::Player ? 0.82f : 0.42f, 0.28f);
    }

    for (size_t i = 0; i < game.collectibles().size(); ++i) {
        const Collectible& coin = game.collectibles()[i];
        const float bob = std::sin(coin.bobTime * 4.0f) * 0.08f;
        addRect(coin.x - 0.11f, coin.y + bob - 0.11f, 0.22f, 0.22f,
                0.98f, 0.78f, 0.22f);
    }
}

void drawHud(const Game& game, float mouseX, float mouseY)
{
    addRect(0.0f, 5.57f, 24.0f, 0.43f, 0.06f, 0.12f, 0.13f, 0.86f);
    addText(0.32f, 5.75f, "GARDEN", 0.065f, 0.91f, 0.89f, 0.67f);
    addText(3.0f, 5.75f,
            "COINS " + std::to_string(game.economy().coins()), 0.065f,
            0.98f, 0.80f, 0.31f);
    std::string waveText = "WAVE ";
    if (game.waves().currentWave() >= 0)
        waveText += std::to_string(game.waves().currentWave() + 1);
    else
        waveText += "READY";
    waveText += " / " + std::to_string(game.waves().waveCount());
    addText(6.5f, 5.75f, waveText, 0.065f, 0.83f, 0.90f, 0.75f);
    addText(12.1f, 5.75f,
            "ENEMIES " + std::to_string(game.aliveEnemyCount()), 0.065f,
            0.95f, 0.70f, 0.61f);
    addText(19.0f, 5.75f, "ESC  PAUSE", 0.055f, 0.76f, 0.83f, 0.78f);

    addRect(0.0f, 0.0f, 24.0f, 0.78f, 0.06f, 0.12f, 0.13f, 0.84f);
    addText(0.28f, 0.46f, "1-3 / CLICK CARD", 0.055f, 0.84f, 0.90f, 0.77f);
    addText(0.28f, 0.17f, "THEN CLICK A LANE", 0.045f, 0.63f, 0.76f, 0.68f);
    const PlayerType cardTypes[] = {PlayerType::Melee, PlayerType::Ranged, PlayerType::Defense};
    const char* cardNames[] = {"MELEE", "RANGED", "DEFENSE"};
    for (int i = 0; i < 3; ++i) {
        const float x = 7.0f + i * 3.25f;
        const bool unlocked = game.economy().isUnlocked(cardTypes[i]);
        const bool armed = game.armedCard() == i;
        const bool hovered = mouseX >= x && mouseX <= x + 2.9f &&
                             mouseY >= 0.08f && mouseY <= 0.72f;
        addRect(x, 0.10f, 2.9f, 0.56f,
                armed ? 0.25f : (hovered && unlocked ? 0.20f : 0.12f),
                armed ? 0.48f : (hovered && unlocked ? 0.39f : 0.24f),
                armed ? 0.25f : (hovered && unlocked ? 0.25f : 0.22f));
        addRect(x, 0.10f, 0.07f, 0.56f, unlocked ? 0.72f : 0.34f,
                unlocked ? 0.82f : 0.36f, unlocked ? 0.42f : 0.36f);
        const float textColor = unlocked ? 0.91f : 0.46f;
        addText(x + 0.20f, 0.41f, cardNames[i], 0.052f,
                textColor, textColor, textColor);
        const std::string detail = unlocked
            ? "COST " + std::to_string(Economy::costOf(cardTypes[i]))
            : "LOCKED";
        addText(x + 0.20f, 0.19f, detail, 0.043f,
                unlocked ? 0.91f : 0.58f, unlocked ? 0.75f : 0.58f, 0.42f);
    }
    addText(17.25f, 0.40f,
            "LANES SELECTED " + std::to_string(game.playfield().selectedLaneCount()),
            0.046f, 0.79f, 0.87f, 0.76f);
    addText(17.25f, 0.17f, "W/UP JUMP  S/DOWN DUCK", 0.036f,
            0.79f, 0.87f, 0.76f);
}

void drawMenu(const Game& game)
{
    const Menu& menu = game.menu();
    const GameState state = game.state();
    const bool mainMenu = state == GameState::MainMenu;

    if (mainMenu) {
        drawGardenBackdrop();
        addText(1.0f, 4.70f, "GARDEN", 0.20f, 0.95f, 0.89f, 0.53f);
        addText(1.0f, 4.10f, "GUARDIANS", 0.16f, 0.76f, 0.92f, 0.67f);
        addText(1.0f, 3.63f, "A LITTLE STRATEGY", 0.065f, 0.81f, 0.88f, 0.79f);
        addText(1.0f, 3.30f, "A LOT OF HEART.", 0.065f, 0.81f, 0.88f, 0.79f);
        addText(1.0f, 0.42f, "BUILD YOUR TEAM. PROTECT EVERY LANE.", 0.050f,
                0.81f, 0.88f, 0.72f);
    }
    else {
        drawBattlefield(game);
        drawHud(game, g_input.mouseX, g_input.mouseY);
        addRect(0.0f, 0.0f, 24.0f, 6.0f, 0.035f, 0.055f, 0.065f, 0.72f);
    }

    const float panelX = 13.0f;
    addRect(panelX + 0.12f, 0.48f, 9.1f, 5.08f, 0.035f, 0.09f, 0.09f);
    addRect(panelX, 0.60f, 9.1f, 5.08f, 0.12f, 0.20f, 0.17f);
    addRect(panelX + 0.10f, 0.70f, 8.9f, 4.88f, 0.08f, 0.15f, 0.14f);

    std::string heading;
    std::string subtitle;
    if (state == GameState::MainMenu) {
        heading = menu.kind() == MenuKind::SoundSettings ? "SOUND" : "WELCOME";
        subtitle = menu.kind() == MenuKind::SoundSettings
            ? "MAKE IT COMFORTABLE" : "READY WHEN YOU ARE";
    }
    else if (state == GameState::Paused) {
        heading = "TAKE A BREATHER";
        subtitle = "YOUR GARDEN IS SAFE FOR NOW";
    }
    else if (state == GameState::Won) {
        heading = "YOU DID IT!";
        subtitle = "EVERY LANE IS SAFE";
    }
    else {
        heading = "GARDEN LOST";
        subtitle = "THEY MADE IT TO YOUR GATE";
    }
    addText(panelX + 0.62f, 4.66f, heading, 0.095f,
            0.96f, 0.88f, 0.56f);
    addText(panelX + 0.62f, 4.27f, subtitle, 0.047f,
            0.72f, 0.85f, 0.74f);
    addRect(panelX + 0.62f, 4.05f, 7.7f, 0.025f, 0.33f, 0.48f, 0.31f);

    for (int i = 0; i < menu.itemCount(); ++i) {
        const float y = menuRowY(i);
        const bool selected = menu.selectedIndex() == i;
        addRect(panelX + 0.48f, y - 0.13f, 8.05f, 0.62f,
                selected ? 0.24f : 0.11f,
                selected ? 0.40f : 0.22f,
                selected ? 0.23f : 0.18f);
        if (selected)
            addRect(panelX + 0.48f, y - 0.13f, 0.08f, 0.62f,
                    0.95f, 0.78f, 0.32f);
        addText(panelX + 0.82f, y + 0.07f, menu.itemLabel(i), 0.064f,
                selected ? 0.98f : 0.80f,
                selected ? 0.96f : 0.87f,
                selected ? 0.74f : 0.80f);
        addText(panelX + 7.98f, y + 0.08f, ">", 0.062f,
                selected ? 0.98f : 0.52f,
                selected ? 0.83f : 0.62f,
                selected ? 0.42f : 0.56f);
    }
    addText(panelX + 0.62f, 0.88f, "UP / DOWN  MOVE     ENTER  SELECT",
            0.043f, 0.69f, 0.79f, 0.70f);
    if (mainMenu)
        addText(panelX + 0.62f, 0.61f, "MOUSE WORKS TOO", 0.042f,
                0.54f, 0.69f, 0.62f);
    else
        addText(panelX + 0.62f, 0.61f, "ESC  " +
                std::string(state == GameState::Paused ? "RESUME" : "MENU"),
                0.042f, 0.54f, 0.69f, 0.62f);
}

void drawCutscene(const Game& game)
{
    drawGardenBackdrop();
    addRect(2.0f, 0.70f, 20.0f, 1.25f, 0.05f, 0.10f, 0.10f);
    addText(2.6f, 1.42f, "A QUIET GARDEN NEEDS A BRAVE TEAM", 0.075f,
            0.94f, 0.90f, 0.69f);
    addText(2.6f, 1.02f, game.cutscene().currentSubtitle(), 0.052f,
            0.84f, 0.91f, 0.82f);
    addText(19.2f, 5.60f, "ESC  SKIP", 0.052f, 0.84f, 0.91f, 0.82f);
}

void drawGame(const Game& game, float mouseX, float mouseY,
              GLuint vao, GLuint vbo)
{
    if (game.state() == GameState::MainMenu ||
        game.state() == GameState::Paused ||
        game.state() == GameState::Won ||
        game.state() == GameState::Lost) {
        drawMenu(game);
    }
    else if (game.state() == GameState::Cutscene) {
        drawCutscene(game);
    }
    else {
        drawBattlefield(game);
        drawHud(game, mouseX, mouseY);
    }
    flushShapes(vao, vbo);
}

int main()
{
    if (!glfwInit()) return -1;

    // --- PART 3 (platform guard) ------------------------------------------
    // macOS only exposes OpenGL 4.1 (and no 4.3 context), Windows keeps the
    // 4.3 core profile of the labs.
#ifdef __APPLE__
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#else
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif
    // -----------------------------------------------------------------------

    GLFWwindow* window = glfwCreateWindow(g_screenWidth, g_screenHeight,
                                          "Project1 - 2D Game", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);   // v-sync

    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetKeyCallback(window, keyCallback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to init GLAD\n";
        return -1;
    }

    glfwGetFramebufferSize(window, &g_screenWidth, &g_screenHeight);
    glViewport(0, 0, g_screenWidth, g_screenHeight);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // the labs' shader program, ready for the rendering step
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    int success;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        std::cerr << "Shader link error:\n" << infoLog << std::endl;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    GLuint uiVao = 0;
    GLuint uiVbo = 0;
    glGenVertexArrays(1, &uiVao);
    glGenBuffers(1, &uiVbo);
    glBindVertexArray(uiVao);
    glBindBuffer(GL_ARRAY_BUFFER, uiVbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float),
                          (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 7 * sizeof(float),
                          (void*)(6 * sizeof(float)));
    glBindVertexArray(0);

    // camera of the 2D scene: the frontend only ever builds MODEL matrices for
    // the lanes / characters / UI, the view and the projection stay like this
    GLint locModel      = glGetUniformLocation(shaderProgram, "u_Model");
    GLint locView       = glGetUniformLocation(shaderProgram, "u_View");
    GLint locProjection = glGetUniformLocation(shaderProgram, "u_Projection");

    glm::mat4 view       = glm::mat4(1.0f);      // side view, no camera movement
    glm::mat4 model      = glm::mat4(1.0f);

    // --- PART 3: the scene renderer may only be created once GL is loaded --
    if (!g_sceneRenderer.init()) {
        std::cerr << "Failed to initialise the scene renderer\n";
        glfwTerminate();
        return -1;
    }
    // ------------------------------------------------------------------------

    // start the cursor in the middle of the window (window size, not
    // framebuffer size - see screen <-> logic space above)
    int windowWidth = 0;
    int windowHeight = 0;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);
    cursorPosCallback(window, windowWidth * 0.5, windowHeight * 0.5);

    std::cout
        << "Controls:\n"
        << "  Menus                  : Hover and click, or arrows + Enter; Esc pauses / resumes\n"
        << "  Click a lane           : select / deselect it (jump & duck are PER SELECTED LANE)\n"
        << "  Cards                  : click a card or press 1 / 2 / 3, then click a lane\n"
        << "  W or Up / S or Down    : jump / duck the players of the selected lanes\n"
        << "  Q                      : quit the window\n\n";

    float lastFrame = static_cast<float>(glfwGetTime());

    while (!glfwWindowShouldClose(window))
    {
        // Delta time, exactly like the labs: the logic is told how long the
        // previous frame took so it stays framerate independent.
        float currentFrame = static_cast<float>(glfwGetTime());
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // --- PART 3: rebuilt every frame, so resizing the window keeps the
        // units square and the mouse mapping in sync with the projection ----
        glm::mat4 projection = makeProjection();
        // --------------------------------------------------------------------
        // ----------------------------------------------------------------
        // 1. hand the frame to the game logic
        // ----------------------------------------------------------------
        const GameState currentState = g_game.state();
        const bool menuOpen = currentState == GameState::MainMenu ||
                              currentState == GameState::Paused ||
                              currentState == GameState::Won ||
                              currentState == GameState::Lost;
        if (!handleScreenControlClick(currentState, g_input)) {
            if (menuOpen) {
                if (g_input.mouseMoved)
                    g_input.menuItemHovered = menuHitTest(g_input.mouseX, g_input.mouseY,
                                                          g_game.menu());
                if (g_input.leftClickPressed)
                    g_input.menuItemClicked = menuHitTest(g_input.mouseX, g_input.mouseY,
                                                          g_game.menu());
            }
            else if (currentState == GameState::Playing && g_input.leftClickPressed) {
                const int cardIndex = cardHitTest(g_input.mouseX, g_input.mouseY);
                if (cardIndex == 0) g_game.selectCard(PlayerType::Melee);
                if (cardIndex == 1) g_game.selectCard(PlayerType::Ranged);
                if (cardIndex == 2) g_game.selectCard(PlayerType::Defense);

                if (g_input.mouseY < 0.78f)
                    g_input.leftClickPressed = false;
            }
        }
        g_game.update(deltaTime, g_input);

        // Esc only closes the window on the main menu - during play it is the
        // pause key the description asks for.
        if (g_input.escapePressed && g_game.state() == GameState::MainMenu)
            glfwSetWindowShouldClose(window, true);

        g_input.clearEdges();

        printEvents();
        printCutsceneLine();

        // the window title doubles as a tiny HUD (coins, wave, enemies, ...)
        std::string title = std::string("Project1 - 2D Game | ") + g_game.statusLine();
        glfwSetWindowTitle(window, title.c_str());

        // ----------------------------------------------------------------
        // 2. draw the scene and the current screen's interface
        // ----------------------------------------------------------------
        glClearColor(0.04f, 0.08f, 0.10f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // --- PART 3: the visual layer draws the whole scene ----------------
        // The world goes first: there is no depth test, so whatever is drawn
        // last ends up on top, and the interface must sit above the world.
        g_sceneRenderer.render(g_game, static_cast<float>(glfwGetTime()), projection,
                               viewHeight());
        // --------------------------------------------------------------------

        // OpenGL remembers its state between draws, and the scene renderer
        // leaves its own shader and possibly additive blending active.
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glUseProgram(shaderProgram);
        glUniformMatrix4fv(locModel,      1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(locView,       1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(locProjection, 1, GL_FALSE, glm::value_ptr(projection));
        drawGame(g_game, g_input.mouseX, g_input.mouseY, uiVao, uiVbo);

        // ----------------------------------------------------------------
        // 3. present
        // ----------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();

        if (g_game.quitRequested())
            glfwSetWindowShouldClose(window, true);
    }

    // --- PART 3: release the scene renderer before the context goes away ----
    g_sceneRenderer.shutdown();
    // ------------------------------------------------------------------------
    glDeleteBuffers(1, &uiVbo);
    glDeleteVertexArrays(1, &uiVao);
    glDeleteProgram(shaderProgram);
    glfwTerminate();
    return 0;
}
