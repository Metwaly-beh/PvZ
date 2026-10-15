// ---------------------------------------------------------------------------
// main.cpp
//
// The OpenGL host of the project: GLFW + GLAD with the same boot sequence as
// the labs. Nothing of the game is drawn yet, this file only
//   1. turns the GLFW callbacks into an InputState (edges, like Lab3/Lab4),
//   2. calls g_game.update(deltaTime, g_input) once per frame,
//   3. prints the events the logic reports and keeps a tiny HUD in the title.
//
// Everything that will be drawn later (lanes, characters, cards, health bars,
// cutscene actors, menus) reads the read-only accessors of Game:
//      const Playfield& grid = g_game.playfield();
//      const std::vector<Enemy>& enemies = g_game.enemies();
// ---------------------------------------------------------------------------

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <string>
#include <vector>
#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Game.h"

int g_screenWidth = 1280;
int g_screenHeight = 720;

Game       g_game;
InputState g_input;

// ---------------------------------------------------------------------------
// shaders (identical style to Lab2 Template / Lab4Camera)
// ---------------------------------------------------------------------------

const char* vertexShaderSource = R"(
#version 430 core

layout (location = 0) in vec3 aPos;     
layout (location = 1) in vec3 aColor;   

out vec3 vColor;                        

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Projection;

void main()
{
    gl_Position = u_Projection * u_View * u_Model * vec4(aPos, 1.0);

    vColor = aColor;
}
)";

const char* fragmentShaderSource = R"(
#version 430 core

in vec3 vColor;
out vec4 FragColor;

void main()
{
    FragColor = vec4(vColor, 1.0);
}
)";

// ---------------------------------------------------------------------------
// screen <-> logic space
//
// The gameplay lives in PLAYFIELD_WIDTH x PLAYFIELD_HEIGHT logic units with y
// pointing UP. The window shows exactly that rectangle, so the mapping is a
// plain linear conversion in (and a glm::ortho in the shader).
// ---------------------------------------------------------------------------

float screenToLogicX(double xpos)
{
    return static_cast<float>(xpos / g_screenWidth) * PLAYFIELD_WIDTH;
}

float screenToLogicY(double ypos)
{
    return (1.0f - static_cast<float>(ypos / g_screenHeight)) * PLAYFIELD_HEIGHT;
}

// Same idea as the ortho camera of Lab4: x grows to the right, y grows up, and
// one logic unit is one unit on screen, so the renderer can pass the positions
// of the logic straight to the shader.
glm::mat4 makeProjection()
{
    return glm::ortho(0.0f, PLAYFIELD_WIDTH, 0.0f, PLAYFIELD_HEIGHT, -1.0f, 1.0f);
}

void framebufferSizeCallback(GLFWwindow* /*window*/, int width, int height)
{
    g_screenWidth = width;
    g_screenHeight = height;
    glViewport(0, 0, width, height);
}

void cursorPosCallback(GLFWwindow* /*window*/, double xpos, double ypos)
{
    g_input.mouseX = screenToLogicX(xpos);
    g_input.mouseY = screenToLogicY(ypos);
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

int main()
{
    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

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

    // camera of the 2D scene: the frontend only ever builds MODEL matrices for
    // the lanes / characters / UI, the view and the projection stay like this
    GLint locModel      = glGetUniformLocation(shaderProgram, "u_Model");
    GLint locView       = glGetUniformLocation(shaderProgram, "u_View");
    GLint locProjection = glGetUniformLocation(shaderProgram, "u_Projection");

    glm::mat4 projection = makeProjection();
    glm::mat4 view       = glm::mat4(1.0f);      // side view, no camera movement
    glm::mat4 model      = glm::mat4(1.0f);

    // start the cursor in the middle of the window
    cursorPosCallback(window, g_screenWidth * 0.5, g_screenHeight * 0.5);

    std::cout
        << "Controls:\n"
        << "  Main menu / pause menu : Arrow keys = move, Enter = confirm, Esc = pause / resume\n"
        << "  Click a lane           : select / deselect it (jump & duck are PER SELECTED LANE)\n"
        << "  1 / 2 / 3              : arm the Melee / Ranged / Defense card, then click a lane\n"
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

        // ----------------------------------------------------------------
        // 1. hand the frame to the game logic
        // ----------------------------------------------------------------
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
        // 2. clear the screen. This is where the frontend will draw
        //    g_game.playfield(), g_game.players(), g_game.enemies(), ...
        // ----------------------------------------------------------------
        glClearColor(0.10f, 0.10f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);
        glUniformMatrix4fv(locModel,      1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(locView,       1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(locProjection, 1, GL_FALSE, glm::value_ptr(projection));
        // (frontend drawing goes here: one draw call per lane / player / enemy)

        // ----------------------------------------------------------------
        // 3. present
        // ----------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();

        if (g_game.quitRequested())
            glfwSetWindowShouldClose(window, true);
    }

    glDeleteProgram(shaderProgram);
    glfwTerminate();
    return 0;
}
