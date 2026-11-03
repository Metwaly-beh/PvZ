#ifndef CORE_INPUT_H
#define CORE_INPUT_H

// ---------------------------------------------------------------------------
// Input.h
//
// The ONLY thing the game logic knows about input.
//
// Button fields are edges: they are true for exactly one update() call and
// then the host clears them. Mouse coordinates and UI hit targets are refreshed
// each frame. This matches GLFW_PRESS semantics for menu and gameplay actions.
//
// Mouse coordinates are already converted to LOGIC space by the host:
//   mouseX in [0, PLAYFIELD_WIDTH],  mouseY in [0, PLAYFIELD_HEIGHT] (y up).
// ---------------------------------------------------------------------------

struct InputState
{
    // mouse position (logic units) - refreshed every frame, not an edge
    float mouseX;
    float mouseY;
    bool mouseMoved;

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

    // UI hit targets are resolved by the frontend and consumed by menus.
    int menuItemHovered;
    int menuItemClicked;

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
        mouseMoved = false;
        leftClickPressed = false;
        wPressed = sPressed = upPressed = downPressed = false;
        enterPressed = escapePressed = false;
        menuItemHovered = menuItemClicked = -1;
        card1Pressed = card2Pressed = card3Pressed = false;
    }

    // Called by the host after Game::update() consumed the frame.
    void clearEdges()
    {
        leftClickPressed = false;
        mouseMoved = false;
        wPressed = sPressed = upPressed = downPressed = false;
        enterPressed = escapePressed = false;
        menuItemHovered = menuItemClicked = -1;
        card1Pressed = card2Pressed = card3Pressed = false;
    }

    bool jumpPressed() const { return wPressed || upPressed; }
    bool duckPressed() const { return sPressed || downPressed; }
};

#endif // CORE_INPUT_H
