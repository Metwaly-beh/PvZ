#ifndef MENUS_H
#define MENUS_H

// ---------------------------------------------------------------------------
// Menus.h  (section 4.3 - Game Menus)
//
// This is LOGIC only: which entry is highlighted and which ACTION pressing
// Enter produces. Drawing the menus is the frontend's job - it can read
// itemCount() / selectedIndex() / itemLabel(i) and render them however it
// wants (text is data here, nothing is drawn).
//
//   Main menu  : Start the game, Open the sound settings, Exit
//   Pause menu : Continue, Restart, Return to the Main Menu     (Esc during play)
//   Win/Lose   : Restart, Return to the Main Menu
//   Sound      : Mute/Unmute, Volume up, Volume down, Back      (bonus)
// ---------------------------------------------------------------------------

#include "Core/Events.h"
#include "Core/Input.h"
#include "Core/Types.h"

enum class MenuKind
{
    Main,
    Pause,
    WinLose,
    SoundSettings
};

// what the confirmed (Enter) entry asks the game to do
enum class MenuAction
{
    None,
    Start,              // Main menu: cutscene, then the game starts
    Resume,             // Pause menu: continue the game
    Restart,            // fresh world, straight into gameplay
    GoToMainMenu,
    Exit,               // close the application
    OpenSoundSettings,  // bonus
    Back,               // leave the sound settings menu
    ToggleMute,
    VolumeUp,
    VolumeDown
};

class Menu
{
public:
    Menu();

    void open(MenuKind kind);
    void close() { m_open = false; }

    bool      isOpen() const { return m_open; }
    MenuKind  kind() const { return m_kind; }

    // feeds one input frame; returns the confirmed action (or None)
    MenuAction handleInput(const InputState& in);

    // --- everything a frontend needs to draw the menu ---------------------
    int         itemCount() const;
    int         selectedIndex() const { return m_selected; }
    const char* itemLabel(int index) const;

private:
    MenuAction actionOf(int index) const;

    bool      m_open;
    MenuKind  m_kind;
    int       m_selected;
};

#endif // MENUS_H
