#ifndef CORE_TYPES_H
#define CORE_TYPES_H

// ---------------------------------------------------------------------------
// Types.h
//
// Everything the gameplay logic shares: the tiny math type, the enums that
// describe what an object IS, and every tunable constant of the game.
//
// The values are taken straight from the project description document so that
// the whole balance of the game can be tweaked in ONE place. Nothing in here
// touches OpenGL - this is pure logic (the frontend reads these values and
// decides how to draw them).
//
// Coordinate system (side view):
//
//      y ^   +--------------------+  PLAYFIELD_HEIGHT
//        |   |  lane 3 (top)      |
//        |   |  lane 2            |     x grows to the RIGHT, so
//        |   |  lane 1            |     players advance with +x and
//        |   |  lane 0 (bottom)   |     enemies advance with -x.
//        +---+--------------------+--> x
//            0  HOME_BASE_X     PLAYFIELD_WIDTH
// ---------------------------------------------------------------------------

#include <cmath>
#include <string>

// --- tiny math type --------------------------------------------------------

struct Vec2
{
    float x;
    float y;

    Vec2() : x(0.0f), y(0.0f) {}
    Vec2(float x_, float y_) : x(x_), y(y_) {}

    Vec2 operator+(const Vec2& o) const { return Vec2(x + o.x, y + o.y); }
    Vec2 operator-(const Vec2& o) const { return Vec2(x - o.x, y - o.y); }
    Vec2 operator*(float s)       const { return Vec2(x * s, y * s); }
};

inline float distance(const Vec2& a, const Vec2& b)
{
    float dx = b.x - a.x;
    float dy = b.y - a.y;
    return std::sqrt(dx * dx + dy * dy);
}

inline float clampf(float v, float lo, float hi)
{
    return (v < lo) ? lo : ((v > hi) ? hi : v);
}

// --- enums -----------------------------------------------------------------

// Which screen of the application we are on (a classic state machine).
enum class GameState
{
    MainMenu,
    Cutscene,
    Playing,
    Paused,
    Won,
    Lost
};

// The three playable characters (project description, section 3.2).
enum class PlayerType
{
    Melee,      // close range, advances, 5 lives, 1 damage per attack
    Ranged,     // long range projectile, advances, 5 lives, 1 damage per attack
    Defense     // NEVER moves, 7 lives, absorbs attacks, protects players behind it
};

// The three enemy characters (section 3.3).
enum class EnemyType
{
    EasyUpper,  // ranged, shots come FROM ABOVE  -> dodge with DUCK, 5 lives, 1 dmg
    MediumLower,// ranged, shots come FROM BELOW  -> dodge with JUMP, 5 lives, 1 dmg
    HardMelee   // runs at the player, close range, 7 lives, 2 dmg per attack
};

// What a character is currently doing (drives animation on the frontend too).
enum class CharState
{
    Idle,
    Advancing,   // walking along its lane
    Attacking,   // swinging / firing
    Fighting,    // stopped because it met the other side, trading blows
    Jumping,     // in the air (dodges LOW attacks)
    Ducking,     // crouched  (dodges HIGH attacks)
    Dead
};

// Height of an attack - decides WHICH defensive move dodges it:
//   jumping avoids LOW attacks, ducking avoids HIGH attacks (section 3.8).
enum class AttackHeight
{
    High,      // shot from above  -> duck to avoid
    Low,       // shot from below  -> jump to avoid
    Straight   // melee / straight shot -> cannot be jumped or ducked
};

// Which side a projectile belongs to.
enum class Faction
{
    Player,
    Enemy
};

// Lifecycle of one wave (section 3.3):
//   NotStarted -> Spawning -> Clearing (all spawns done, clean the screen) -> next wave
enum class WavePhase
{
    NotStarted,
    Spawning,
    Clearing
};

// --- playfield geometry (logic units; the frontend maps them to pixels) ----

const float PLAYFIELD_WIDTH  = 24.0f;
const int   LANE_COUNT       = 4;      // spec requires a MINIMUM of three lanes
const float LANE_HEIGHT      = 1.5f;
const float PLAYFIELD_HEIGHT = LANE_COUNT * LANE_HEIGHT;

const float HOME_BASE_X      = 2.0f;   // an enemy reaching this x loses the game
const float PLAYER_SPAWN_X   = 2.5f;   // leftmost placement column of a lane
const float ENEMY_SPAWN_X    = 23.5f;  // right edge, where enemies emerge
const float PLAYFIELD_RIGHT  = 23.5f;  // players stop advancing here

// --- movement --------------------------------------------------------------

const float PLAYER_ADVANCE_SPEED = 1.2f;   // players walk towards the right
const float ENEMY_EASY_SPEED     = 1.0f;   // "normal speed"
const float ENEMY_MEDIUM_SPEED   = 1.0f;   // "normal speed"
const float ENEMY_HARD_SPEED     = 1.7f;   // "runs toward the player"

// --- combat ----------------------------------------------------------------

const float ENCOUNTER_DISTANCE   = 0.75f;  // meet & collide -> both stop and fight
const float MELEE_RANGE          = 1.0f;   // close range weapons
const float RANGED_RANGE         = 12.0f;  // long range weapons - also how far a
                                           // fired projectile travels before it dies
const float PROJECTILE_SPEED     = 7.0f;   // world units per second
const float PROJECTILE_HIT_RADIUS= 0.45f;  // projectile <-> character overlap
const float PICKUP_RADIUS        = 0.6f;   // player <-> collectible overlap
// Both sides attack continuously from the moment they exist (page 4: they
// "immediately start attacking ... even if there are no enemies" / "no
// players"), so a duel is decided by this ratio: a player attacks 4.6x as
// often as an enemy. Tuned with the auto-play sweep: a Melee-only policy wins
// 38 of 40 seeds at these values.
const float PLAYER_ATTACK_INTERVAL = 0.5f; // seconds between player attacks
const float ENEMY_ATTACK_INTERVAL  = 2.3f; // seconds between enemy attacks
const float HIT_FLASH_TIME       = 0.25f;  // frontend tints red while hitFlash > 0

const int PLAYER_DAMAGE     = 1;   // every player type deals 1 (section 3.2)
const int ENEMY_DAMAGE      = 1;   // easy & medium deal 1 (section 3.3)
const int HARD_ENEMY_DAMAGE = 2;   // hard melee deals 2

// --- jump / duck (section 3.8) ---------------------------------------------

const float JUMP_TIME = 0.6f;      // how long a jump lasts (seconds)
const float DUCK_TIME  = 0.6f;      // how long a duck lasts (seconds)

// --- health (players table page 9, enemy list page 10) ---------------------

const int MELEE_HP  = 5;
const int RANGED_HP = 5;
const int DEFENSE_HP = 7;          // "7 hits"
const int EASY_HP   = 5;
const int MEDIUM_HP = 5;
const int HARD_HP   = 7;

// --- economy (page 5/6, page 9 notes column) -------------------------------

const int STARTING_COINS = 30;     // "enough to purchase ... at least three Melee"
const int MELEE_COST     = 10;     // 30 / 3
const int RANGED_COST    = 20;     // "2x the collectibles required to unlock Melee"
const int DEFENSE_COST   = 30;     // "3x the collectibles required to unlock Melee"

// Values of the collectibles enemies drop when defeated. Four lanes cannot be
// covered with the 30 starting coins (3 melee players), so a couple of easy
// kills must be enough to pay for the next defender.
// (The description does not fix these numbers - it only says the coin note in
//  the text is not accurate and that 30 coins buys three Melee players.)
const int COINS_EASY_DROP   = 10;
const int COINS_MEDIUM_DROP = 15;
const int COINS_HARD_DROP   = 25;

// --- waves (page 10) -------------------------------------------------------

const int WAVE1_MIN_ENEMIES = 3;    // easy only
const int WAVE2_MIN_ENEMIES = 5;    // easy + medium
const int WAVE3_MIN_ENEMIES = 10;   // easy + medium + hard

const float WAVE_SPAWN_INTERVAL = 2.5f;   // seconds between two spawns of a wave

// --- cutscene (page 2: "10-20 second cutscene") ----------------------------

const float CUTSCENE_DURATION = 14.0f;

// --- helpers ---------------------------------------------------------------

inline const char* toString(GameState s)
{
    switch (s) {
    case GameState::MainMenu:  return "MainMenu";
    case GameState::Cutscene:  return "Cutscene";
    case GameState::Playing:   return "Playing";
    case GameState::Paused:    return "Paused";
    case GameState::Won:       return "Won";
    case GameState::Lost:      return "Lost";
    }
    return "?";
}

inline const char* toString(PlayerType t)
{
    switch (t) {
    case PlayerType::Melee:   return "Melee";
    case PlayerType::Ranged:  return "Ranged";
    case PlayerType::Defense: return "Defense";
    }
    return "?";
}

inline const char* toString(EnemyType t)
{
    switch (t) {
    case EnemyType::EasyUpper:   return "EasyUpper";
    case EnemyType::MediumLower: return "MediumLower";
    case EnemyType::HardMelee:   return "HardMelee";
    }
    return "?";
}

#endif // CORE_TYPES_H
