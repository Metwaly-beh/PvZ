#include <iostream>
#include <fstream>
#include <vector>
#include <iomanip>

#include "../Core/Types.h"
#include "../Core/Input.h"
#include "../Game.h"
#include "../Playfield.h"
#include "../Player.h"
#include "../Enemy.h"
#include "../Projectile.h"
#include "../Collectibles.h"
#include "../Economy.h"
#include "../Waves.h"
#include "../Menus.h"
#include "../Cutscene.h"

#include "../Render/Canvas.h"
#include "../Render/Environment.h"
#include "../Render/CharacterSprites.h"

// ---------------------------------------------------------------------------
// Tests/render_dump.cpp
//
// a headless renderer that drives the game and dumps vertices to a file.
// ---------------------------------------------------------------------------

void dumpCanvas(const std::string& filename, const Canvas& canvas, float viewHeight) {
    std::ofstream out(filename);
    if (!out) {
        std::cerr << "Failed to open " << filename << " for writing\n";
        return;
    }
    out << "viewHeight " << viewHeight << "\n";
    const auto& verts = canvas.vertices();
    out << "vertices " << verts.size() << "\n";
    for (const auto& v : verts) {
        out << std::fixed << std::setprecision(6)
            << v.x << " " << v.y << " "
            << v.u << " " << v.v << " "
            << v.r << " " << v.g << " " << v.b << " " << v.a << "\n";
    }
    out.close();
}

int main() {
    Game game;
    game.seedRandom(123);

    // 1. Set up game state: enter playing mode
    // Simulation of "Start Game" -> "Skip Cutscene"
    InputState in;
    in.enterPressed = true;
    game.update(0.0f, in);
    in.clearEdges();
    in.escapePressed = true;
    game.update(0.0f, in);
    in.clearEdges();

    // 2. Force a scenario:
    // - Place Melee player in lane 0
    // - Place Ranged player in lane 1
    // - Place Defense player in lane 2
    // - Let a few frames pass so enemies spawn

    // Lane 0: Melee
    in.mouseX = 6.0f; in.mouseY = 0.0f * 1.5f + 0.75f; in.leftClickPressed = true;
    game.update(0.0f, in); in.clearEdges();
    in.card1Pressed = true;
    game.update(0.0f, in); in.clearEdges();

    // Lane 1: Ranged
    in.mouseX = 6.0f; in.mouseY = 1.0f * 1.5f + 0.75f; in.leftClickPressed = true;
    game.update(0.0f, in); in.clearEdges();
    // We need to unlock Ranged first (Economy needs 20 points)
    // Normally we'd earn points, but we can justcheat/manipulate for the dump if we had access
    // Since we don't, we'll just place another Melee in lane 1 for now,
    // or just wait for the wave to spawn and earn points.
    in.card1Pressed = true;
    game.update(0.0f, in); in.clearEdges();

    // Lane 2: Defense
    in.mouseX = 6.0f; in.mouseY = 2.0f * 1.5f + 0.75f; in.leftClickPressed = true;
    game.update(0.0f, in); in.clearEdges();
    in.card1Pressed = true;
    game.update(0.0f, in); in.clearEdges();

    // Step some frames to let enemies spawn and move
    for (int i = 0; i < 60; ++i) {
        game.update(1.0f/60.0f, InputState());
    }

    float viewHeight = 13.5f; // Typical 1280x720
    Canvas solid;
    Canvas light;

    drawEnvironment(game, solid, light, 0.0f, viewHeight);
    drawCharacters(game, solid, 0.0f);

    dumpCanvas("scene_dump.txt", solid, viewHeight);
    dumpCanvas("light_dump.txt", light, viewHeight);

    std::cout << "Dumps written to scene_dump.txt and light_dump.txt\n";

    return 0;
}
