#include "Menus.h"

Menu::Menu()
    : m_open(false), m_kind(MenuKind::Main), m_selected(0)
{
}

void Menu::open(MenuKind kind)
{
    m_kind = kind;
    m_selected = 0;      // every menu starts on its first entry
    m_open = true;
}

int Menu::itemCount() const
{
    switch (m_kind) {
    case MenuKind::Main:          return 3;
    case MenuKind::Pause:         return 3;
    case MenuKind::WinLose:       return 2;
    case MenuKind::SoundSettings: return 4;
    }
    return 0;
}

MenuAction Menu::actionOf(int index) const
{
    switch (m_kind) {
    case MenuKind::Main:
        // the order of section 4.3: Start, Exit, then the bonus sound settings
        if (index == 0) return MenuAction::Start;
        if (index == 1) return MenuAction::Exit;
        if (index == 2) return MenuAction::OpenSoundSettings;
        break;

    case MenuKind::Pause:
        if (index == 0) return MenuAction::Resume;
        if (index == 1) return MenuAction::Restart;
        if (index == 2) return MenuAction::GoToMainMenu;
        break;

    case MenuKind::WinLose:
        if (index == 0) return MenuAction::Restart;
        if (index == 1) return MenuAction::GoToMainMenu;
        break;

    case MenuKind::SoundSettings:
        if (index == 0) return MenuAction::ToggleMute;
        if (index == 1) return MenuAction::VolumeUp;
        if (index == 2) return MenuAction::VolumeDown;
        if (index == 3) return MenuAction::Back;
        break;
    }
    return MenuAction::None;
}

const char* Menu::itemLabel(int index) const
{
    switch (m_kind) {
    case MenuKind::Main:
        if (index == 0) return "Start Game";
        if (index == 1) return "Exit";
        if (index == 2) return "Sound Settings";
        break;

    case MenuKind::Pause:
        if (index == 0) return "Continue";
        if (index == 1) return "Restart";
        if (index == 2) return "Return to Main Menu";
        break;

    case MenuKind::WinLose:
        if (index == 0) return "Restart";
        if (index == 1) return "Return to Main Menu";
        break;

    case MenuKind::SoundSettings:
        if (index == 0) return "Mute / Unmute";
        if (index == 1) return "Volume Up";
        if (index == 2) return "Volume Down";
        if (index == 3) return "Back";
        break;
    }
    return "";
}

MenuAction Menu::handleInput(const InputState& in)
{
    if (!m_open) return MenuAction::None;

    int count = itemCount();
    if (count <= 0) return MenuAction::None;

    // GLFW_PRESS edges only: one step per key press, like Lab3's notes explain
    if (in.upPressed) {
        m_selected = (m_selected - 1 + count) % count;
    }
    if (in.downPressed) {
        m_selected = (m_selected + 1) % count;
    }

    if (in.enterPressed) {
        return actionOf(m_selected);
    }
    return MenuAction::None;
}
