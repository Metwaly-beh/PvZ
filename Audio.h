#ifndef AUDIO_H
#define AUDIO_H

// ---------------------------------------------------------------------------
// Audio.h  (section 5 - BONUS)
//
// The bonus asks for background music, sound effects for at least 4 user
// interactions, and a sound settings menu that can really change volume /
// mute. Audio FILES are assets, and assets are out of scope here - so this
// header defines the INTERFACE the game logic talks to.
//
//   * Game holds a SoundSystem and feeds every GameEvent to it.
//   * Menu volume / mute actions go through setVolume() / toggleMute().
//   * A real implementation (OpenAL / irrKlang / whatever) later just derives
//     from SoundSystem and loads its sounds - gameplay code does not change.
//
// NullSoundSystem is the default: it keeps the volume/mute state so the menu
// logic is fully testable, but plays nothing.
// ---------------------------------------------------------------------------

#include "Core/Events.h"

class SoundSystem
{
public:
    virtual ~SoundSystem() {}

    // called for every event the game emits (attacks, deaths, clicks, ...)
    virtual void onEvent(const GameEvent& /*event*/) {}

    // background music (bonus requirement)
    virtual void playBackgroundMusic() {}
    virtual void stopBackgroundMusic() {}

    // sound settings menu (bonus requirement)
    void setVolume(float v)  { m_volume = (v < 0.0f) ? 0.0f : ((v > 1.0f) ? 1.0f : v); onVolumeChanged(); }
    void volumeUp()          { setVolume(m_volume + 0.1f); }
    void volumeDown()        { setVolume(m_volume - 0.1f); }
    void toggleMute()        { m_muted = !m_muted; onVolumeChanged(); }

    float volume() const { return m_volume; }
    bool  muted()  const { return m_muted; }

protected:
    virtual void onVolumeChanged() {}

    float m_volume = 1.0f;
    bool  m_muted  = false;
};

// Default "no sound card" implementation - keeps everything testable.
class NullSoundSystem : public SoundSystem
{
};

#endif // AUDIO_H
