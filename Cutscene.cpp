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
    hero.startPos = Vec2(4.8f, 2.9f);
    hero.pos = hero.startPos;
    hero.anim = "idle";

    CutsceneActor foe;
    foe.name = "Raider";
    foe.isPlayer = false;
    foe.startPos = Vec2(19.2f, 2.9f);
    foe.pos = foe.startPos;
    foe.anim = "idle";

    m_actors.push_back(hero);   // actor 0
    m_actors.push_back(foe);    // actor 1

    // The dialogue explains why the garden matters as both characters act.
    m_events.push_back(makeAnim(0.6f, 0, "walk"));
    m_events.push_back(makeMove(0.8f, 2.6f, 0, Vec2(4.8f, 2.9f), Vec2(6.2f, 2.9f)));
    m_events.push_back(makeSubtitle(0.4f, 4.1f, "Sentinel",
        "The garden's seeds will feed the valley through winter."));
    m_events.push_back(makeAnim(1.0f, 1, "walk"));
    m_events.push_back(makeMove(1.0f, 2.2f, 1, Vec2(19.2f, 2.9f), Vec2(18.2f, 2.9f)));
    m_events.push_back(makeAnim(2.7f, 0, "attack"));
    m_events.push_back(makeAnim(3.9f, 0, "idle"));

    // The Raider advances on the garden and threatens its harvest.
    m_events.push_back(makeAnim(4.2f, 1, "walk"));
    m_events.push_back(makeMove(4.2f, 6.3f, 1, Vec2(18.2f, 2.9f), Vec2(11.8f, 2.9f)));
    m_events.push_back(makeSubtitle(4.3f, 7.6f, "Raider",
        "Then I will burn it. My horde is at your gate!"));
    m_events.push_back(makeAnim(6.3f, 1, "attack"));
    m_events.push_back(makeAnim(6.4f, 0, "attack"));
    m_events.push_back(makeAnim(6.7f, 1, "walk"));
    m_events.push_back(makeMove(6.7f, 7.4f, 1, Vec2(11.8f, 2.9f), Vec2(12.7f, 2.9f)));
    m_events.push_back(makeAnim(7.4f, 1, "attack"));

    // Sentinel blocks the charge and drives the Raider back.
    m_events.push_back(makeSubtitle(7.8f, 11.0f, "Sentinel",
        "You will take nothing. This garden is our home. Hold!"));
    m_events.push_back(makeAnim(8.0f, 0, "walk"));
    m_events.push_back(makeMove(8.0f, 9.0f, 0, Vec2(6.2f, 2.9f), Vec2(8.8f, 2.9f)));
    m_events.push_back(makeAnim(9.0f, 0, "attack"));
    m_events.push_back(makeAnim(9.1f, 1, "attack"));
    m_events.push_back(makeAnim(10.0f, 1, "walk"));
    m_events.push_back(makeMove(10.0f, 11.0f, 1, Vec2(12.7f, 2.9f), Vec2(14.0f, 2.9f)));

    // The first raid is about to begin; the defenders take their ground.
    m_events.push_back(makeSubtitle(11.2f, 13.8f, "Narrator",
        "The raid begins. Defend every lane!"));
    m_events.push_back(makeAnim(11.0f, 0, "walk"));
    m_events.push_back(makeMove(11.0f, 12.3f, 0, Vec2(8.8f, 2.9f), Vec2(6.2f, 2.9f)));
    m_events.push_back(makeAnim(11.0f, 1, "walk"));
    m_events.push_back(makeMove(11.0f, 12.5f, 1, Vec2(14.0f, 2.9f), Vec2(18.2f, 2.9f)));
    m_events.push_back(makeAnim(12.3f, 0, "idle"));
    m_events.push_back(makeAnim(12.5f, 1, "idle"));

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
