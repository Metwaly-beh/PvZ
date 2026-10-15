#ifndef CUTSCENE_H
#define CUTSCENE_H

// ---------------------------------------------------------------------------
// Cutscene.h  (section 2 - Cutscene)
//
// "a fully animated, 10-20 second cutscene ... must include at least one
//  player and one enemy, and should briefly establish what the players are
//  defending and why ... must also include a short dialogue or interaction
//  ... must not rely solely on text to convey the story."
//
// The cutscene is therefore a small TIMELINE:
//   * actors  = one player + one enemy (add more later if the theme wants it),
//   * events  = MoveActor / SetAnim / ShowSubtitle / EndScene,
//   * every subtitle is paired with on-screen movement / animation, so the
//     scene can never degrade into "text only".
//
// The logic only produces positions, animation names and subtitle text - the
// frontend draws them.
// ---------------------------------------------------------------------------

#include <string>
#include <vector>

#include "Core/Types.h"

enum class CutsceneEventType
{
    MoveActor,
    SetAnim,
    ShowSubtitle,
    EndScene
};

struct CutsceneEvent
{
    float             startTime;
    float             endTime;
    CutsceneEventType type;
    int               actor;      // index into the actor list, -1 for subtitles
    Vec2              from;
    Vec2              to;
    std::string       anim;       // "walk", "attack", "idle", ...
    std::string       speaker;
    std::string       text;

    CutsceneEvent()
        : startTime(0.0f), endTime(0.0f), type(CutsceneEventType::SetAnim),
          actor(-1), from(), to()
    {
    }
};

struct CutsceneActor
{
    std::string name;
    bool        isPlayer;
    Vec2        pos;      // current position (updated every update)
    Vec2        startPos; // where the actor stands when the scene opens
    std::string anim;
    bool        visible;

    CutsceneActor()
        : isPlayer(true), visible(true)
    {
    }
};

class Cutscene
{
public:
    Cutscene();

    void start();                       // build the default timeline and play it
    void update(float dt);              // advance the timeline

    bool  finished() const { return m_finished; }
    bool  playing()  const { return m_playing; }
    float time()     const { return m_time; }
    float duration() const { return CUTSCENE_DURATION; }

    const std::vector<CutsceneActor>& actors() const { return m_actors; }
    const std::vector<CutsceneEvent>& events() const { return m_events; }

    // the line currently on screen ("" when nobody is talking)
    const std::string& currentSpeaker() const { return m_speaker; }
    const std::string& currentSubtitle() const { return m_subtitle; }

private:
    void buildDefaultTimeline();
    void applyEvents();

    std::vector<CutsceneActor> m_actors;
    std::vector<CutsceneEvent> m_events;

    float       m_time;
    bool        m_playing;
    bool        m_finished;
    std::string m_speaker;
    std::string m_subtitle;
};

#endif // CUTSCENE_H
