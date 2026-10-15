#ifndef CORE_EVENTS_H
#define CORE_EVENTS_H

// ---------------------------------------------------------------------------
// Events.h
//
// The game logic never draws and never plays a sound itself - it EMITS what
// happened and lets the frontend (or the audio system, or the test harness)
// consume it. This keeps the logic 100% OpenGL / UI free while still telling
// the outside world about "a card got unlocked" or "an enemy died".
//
// Every event carries an SfxId: implementing the optional sound bonus then
// becomes a lookup table in the audio layer, with ZERO changes to gameplay.
// ---------------------------------------------------------------------------

#include <string>

enum class SfxId
{
    // gameplay
    LaneSelected,
    LaneDeselected,
    NoLaneSelected,
    PlayerPlaced,
    CardArmed,
    CardLocked,          // card clicked while locked -> must not react
    NotEnoughCoins,
    PlayerJump,
    PlayerDuck,
    PlayerAttack,
    PlayerDied,
    EnemyAttack,
    EnemyDied,
    ProjectileHit,
    CollectiblePicked,
    CardUnlocked,
    CardRelocked,
    WaveStarted,
    WaveCleared,
    // flow
    MenuMoved,
    MenuConfirmed,
    GameWon,
    GameLost,
    CutsceneStarted,
    CutsceneFinished
};

struct GameEvent
{
    SfxId       sfx;
    std::string message;   // human readable detail, printed by the host

    GameEvent() : sfx(SfxId::MenuMoved) {}
    GameEvent(SfxId s, const std::string& m = std::string()) : sfx(s), message(m) {}
};

inline const char* toString(SfxId s)
{
    switch (s) {
    case SfxId::LaneSelected:      return "LaneSelected";
    case SfxId::LaneDeselected:    return "LaneDeselected";
    case SfxId::NoLaneSelected:    return "NoLaneSelected";
    case SfxId::PlayerPlaced:      return "PlayerPlaced";
    case SfxId::CardArmed:         return "CardArmed";
    case SfxId::CardLocked:        return "CardLocked";
    case SfxId::NotEnoughCoins:    return "NotEnoughCoins";
    case SfxId::PlayerJump:        return "PlayerJump";
    case SfxId::PlayerDuck:        return "PlayerDuck";
    case SfxId::PlayerAttack:      return "PlayerAttack";
    case SfxId::PlayerDied:        return "PlayerDied";
    case SfxId::EnemyAttack:       return "EnemyAttack";
    case SfxId::EnemyDied:         return "EnemyDied";
    case SfxId::ProjectileHit:     return "ProjectileHit";
    case SfxId::CollectiblePicked: return "CollectiblePicked";
    case SfxId::CardUnlocked:      return "CardUnlocked";
    case SfxId::CardRelocked:      return "CardRelocked";
    case SfxId::WaveStarted:       return "WaveStarted";
    case SfxId::WaveCleared:       return "WaveCleared";
    case SfxId::MenuMoved:         return "MenuMoved";
    case SfxId::MenuConfirmed:     return "MenuConfirmed";
    case SfxId::GameWon:           return "GameWon";
    case SfxId::GameLost:          return "GameLost";
    case SfxId::CutsceneStarted:   return "CutsceneStarted";
    case SfxId::CutsceneFinished:  return "CutsceneFinished";
    }
    return "?";
}

#endif // CORE_EVENTS_H
