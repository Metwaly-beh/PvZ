#include "Cutscene.h"

namespace {

CutsceneEvent makeSubtitle(float startTime, float endTime,
                           const std::string& speaker, const std::string& text)
{
    CutsceneEvent e;
    e.type = CutsceneEventType::ShowSubtitle;
    e.startTime = startTime;
    e.endTime = endTime;
    e.speaker = speaker;
    e.text = text;
    return e;
}

CutsceneEvent makeMove(float startTime, float endTime, int actor, const Vec2& from, const Vec2& to)
{
    CutsceneEvent e;
    e.type = CutsceneEventType::MoveActor;
    e.startTime = startTime;
    e.endTime = endTime;
    e.actor = actor;
    e.from = from;
    e.to = to;
    return e;
}

CutsceneEvent makeAnim(float time, int actor, const std::string& anim)
{
    CutsceneEvent e;
    e.type = CutsceneEventType::SetAnim;
    e.startTime = time;
    e.endTime = time;
    e.actor = actor;
    e.anim = anim;
    return e;
}

CutsceneEvent makeEnd(float time)
{
    CutsceneEvent e;
    e.type = CutsceneEventType::EndScene;
    e.startTime = time;
    e.endTime = time;
    return e;
}

} // namespace

Cutscene::Cutscene()
    : m_time(0.0f), m_playing(false), m_finished(false)
{
}

void Cutscene::start()
{
    buildDefaultTimeline();
    m_time = 0.0f;
    m_playing = true;
    m_finished = false;
    applyEvents();
}

void Cutscene::buildDefaultTimeline()
{
    m_actors.clear();
    m_events.clear();

    // --- cast: one player and one enemy (both required by section 2) -------
    CutsceneActor hero;
    hero.name = "Sentinel";
    hero.isPlayer = true;
    hero.startPos = Vec2(4.0f, 1.2f);
    hero.pos = hero.startPos;
    hero.anim = "idle";

    CutsceneActor foe;
    foe.name = "Raider";
    foe.isPlayer = false;
    foe.startPos = Vec2(16.0f, 1.2f);
    foe.pos = foe.startPos;
    foe.anim = "idle";

    m_actors.push_back(hero);   // actor 0
    m_actors.push_back(foe);    // actor 1

    // --- beat 1: the Sentinel steps forward while talking ------------------
    // (text is always paired with motion / animation, never text alone)
    m_events.push_back(makeAnim(0.4f, 0, "walk"));
    m_events.push_back(makeAnim(0.4f, 1, "walk"));
    m_events.push_back(makeMove(0.6f, 5.0f, 0, Vec2(4.0f, 1.2f), Vec2(7.4f, 1.2f)));
    m_events.push_back(makeMove(0.6f, 5.4f, 1, Vec2(16.0f, 1.2f), Vec2(11.0f, 1.2f)));
    m_events.push_back(makeSubtitle(0.4f, 4.6f, "Sentinel",
        "The Sunwell Gate has held for a hundred winters. It does not fall today."));

    // --- beat 2: the enemy answers, both sides attack ----------------------
    m_events.push_back(makeAnim(5.0f, 0, "attack"));
    m_events.push_back(makeAnim(5.0f, 1, "attack"));
    m_events.push_back(makeMove(5.2f, 5.8f, 0, Vec2(7.4f, 1.2f), Vec2(8.4f, 1.2f)));
    m_events.push_back(makeMove(5.2f, 5.8f, 1, Vec2(11.0f, 1.2f), Vec2(9.8f, 1.2f)));
    m_events.push_back(makeSubtitle(4.8f, 9.0f, "Raider",
        "Your gate will burn. Nothing you plant here survives the harvest."));

    // --- beat 3: they clash in the middle ---------------------------------
    m_events.push_back(makeMove(6.0f, 6.6f, 0, Vec2(8.4f, 1.2f), Vec2(9.0f, 1.2f)));
    m_events.push_back(makeMove(6.0f, 6.6f, 1, Vec2(9.8f, 1.2f), Vec2(9.2f, 1.2f)));
    m_events.push_back(makeAnim(7.0f, 0, "attack"));
    m_events.push_back(makeAnim(8.0f, 1, "attack"));
    m_events.push_back(makeSubtitle(9.2f, 11.4f, "Sentinel",
        "Then the gate answers for itself. Every lane, every blade - stand fast."));

    // --- beat 4: closing line while both sides fall back ------------------
    m_events.push_back(makeAnim(10.4f, 0, "walk"));
    m_events.push_back(makeAnim(10.4f, 1, "walk"));
    m_events.push_back(makeMove(10.4f, 13.2f, 0, Vec2(9.0f, 1.2f), Vec2(5.6f, 1.2f)));
    m_events.push_back(makeMove(10.4f, 13.4f, 1, Vec2(9.2f, 1.2f), Vec2(15.2f, 1.2f)));
    m_events.push_back(makeSubtitle(11.6f, 13.9f, "Narrator",
        "Three waves. Four lanes. One gate."));
    m_events.push_back(makeAnim(13.4f, 0, "idle"));
    m_events.push_back(makeAnim(13.4f, 1, "idle"));

    // --- end of the scene -> the game starts ------------------------------
    m_events.push_back(makeEnd(CUTSCENE_DURATION));
}

void Cutscene::update(float dt)
{
    if (!m_playing) return;

    m_time += dt;
    applyEvents();

    for (size_t i = 0; i < m_events.size(); ++i) {
        if (m_events[i].type == CutsceneEventType::EndScene && m_time >= m_events[i].startTime) {
            m_finished = true;
            m_playing = false;
            m_speaker.clear();
            m_subtitle.clear();
            return;
        }
    }

    if (m_time >= CUTSCENE_DURATION) {
        m_finished = true;
        m_playing = false;
    }
}

void Cutscene::applyEvents()
{
    // start from the pose the scene opens with
    for (size_t i = 0; i < m_actors.size(); ++i) {
        m_actors[i].pos = m_actors[i].startPos;
        m_actors[i].anim = "idle";
        m_actors[i].visible = true;
    }
    m_speaker.clear();
    m_subtitle.clear();

    for (size_t i = 0; i < m_events.size(); ++i) {
        const CutsceneEvent& e = m_events[i];

        if (e.type == CutsceneEventType::MoveActor && e.actor >= 0 &&
            e.actor < static_cast<int>(m_actors.size())) {
            CutsceneActor& a = m_actors[static_cast<size_t>(e.actor)];
            if (m_time >= e.endTime) {
                a.pos = e.to;                                   // animation done
            }
            else if (m_time > e.startTime) {
                float span = e.endTime - e.startTime;
                float u = (span > 0.0f) ? (m_time - e.startTime) / span : 1.0f;
                a.pos = e.from + (e.to - e.from) * clampf(u, 0.0f, 1.0f);
            }
        }
        else if (e.type == CutsceneEventType::SetAnim && e.actor >= 0 &&
                 e.actor < static_cast<int>(m_actors.size())) {
            if (m_time >= e.startTime)
                m_actors[static_cast<size_t>(e.actor)].anim = e.anim;
        }
        else if (e.type == CutsceneEventType::ShowSubtitle) {
            if (m_time >= e.startTime && m_time <= e.endTime && m_subtitle.empty()) {
                m_speaker = e.speaker;
                m_subtitle = e.text;
            }
        }
    }
}
