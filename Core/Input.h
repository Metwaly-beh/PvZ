#ifndef CORE_INPUT_H
#define CORE_INPUT_H

// ---------------------------------------------------------------------------
// Input.h
//
// The ONLY thing the game logic knows about input.
//
// Every field is an EDGE: it is true for exactly one update() call and then
// the host (main.cpp GLFW callbacks, or the console test harness) clears it
// again. That matches the GLFW_PRESS / GLFW_RELEASE semantics explained in
// Lab3's keyCallback - you want "the key was pressed this frame", not
// "the key is still held", for menu navigation, lane clicks and jump/duck.
//
// Mouse coordinates are already converted to LOGIC space by the host:
//   mouseX in [0, PLAYFIELD_WIDTH],  mouseY in [0, PLAYFIELD_HEIGHT] (y up).
// ---------------------------------------------------------------------------

struct InputState
{
    // mouse position (logic units) - refreshed every frame, not an edge
    float mouseX;
    float mouseY;

    // pointer
    bool leftClickPressed;   // left button went down this frame

    // lane interaction (jump / duck are PER LANE, section 3.8)
    bool wPressed;           // W      -> jump players in the selected lanes
    bool sPressed;           // S      -> duck  players in the selected lanes
    bool upPressed;          // Up     -> same as W (spec allows both)
    bool downPressed;        // Down   -> same as S

    // menus
    bool enterPressed;       // confirm the highlighted menu entry
    bool escapePressed;      // open / close the pause menu

    // player cards (1 = Melee, 2 = Ranged, 3 = Defense).
    // The frontend UI will call Game::selectCard() instead; the keys exist so
    // the game is playable (and testable) without any UI at all.
    bool card1Pressed;
    bool card2Pressed;
    bool card3Pressed;

    InputState()
    {
        mouseX = 0.0f;
        mouseY = 0.0f;
        leftClickPressed = false;
        wPressed = sPressed = upPressed = downPressed = false;
        enterPressed = escapePressed = false;
        card1Pressed = card2Pressed = card3Pressed = false;
    }

    // Called by the host after Game::update() consumed the frame.
    void clearEdges()
    {
        leftClickPressed = false;
        wPressed = sPressed = upPressed = downPressed = false;
        enterPressed = escapePressed = false;
        card1Pressed = card2Pressed = card3Pressed = false;
    }

    bool jumpPressed() const { return wPressed || upPressed; }
    bool duckPressed() const { return sPressed || downPressed; }
};

#endif // CORE_INPUT_H
