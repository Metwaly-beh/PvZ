#ifndef GAME_H
#define GAME_H

// ---------------------------------------------------------------------------
// Game.h
//
// The brain of the project. It owns one instance of every subsystem and runs
// the state machine:
//
//      MainMenu --Start--> Cutscene --14s--> Playing
//      Playing  --Esc----> Paused --> Continue / Restart / Main Menu
//      Playing  --win/lose--> Won / Lost --> Restart / Main Menu
//
// Everything below is PURE LOGIC: no OpenGL calls, no drawing, no sound files.
// The frontend (main.cpp) only
//   * feeds an InputState in,
//   * receives GameEvents out,
//   * reads the read-only accessors to draw whatever it wants.
//
// The per-frame ORDER is the part that must never be shuffled, because the
// requirements depend on it:
//
//   1. grid input (hover / select / place)     3.2 + 3.8
//   2. stance input (jump / duck)              3.8
//   3. waves spawn enemies                     3.3
//   4. players think (attack / advance / stop) 3.2
//   5. enemies think (attack / advance / stop) 3.3
//   6. projectiles fly and hit                 3.2 + 3.3
//   7. deaths -> drops, corpses removed        3.3
//   8. collectibles picked up                  3.3
//   9. card unlocking / re-locking             4.3
//  10. win / lose checks                       3.4
// ---------------------------------------------------------------------------

#include <string>
#include <vector>

#include "Audio.h"
#include "Collectibles.h"
#include "Core/Events.h"
#include "Core/Input.h"
#include "Core/Random.h"
#include "Core/Types.h"
#include "Cutscene.h"
#include "Economy.h"
#include "Enemy.h"
#include "Menus.h"
#include "Playfield.h"
#include "Player.h"
#include "Projectile.h"
#include "Waves.h"

class Game
{
public:
    Game();

    // advances the whole game by one frame
    void update(float dt, const InputState& in);

    // --- frontend / test harness API --------------------------------------
    GameState state() const { return m_state; }
    bool      quitRequested() const { return m_quitRequested; }
    void      requestQuit() { m_quitRequested = true; }

    // exactly what clicking a player card does. Locked or unaffordable cards
    // return false and do NOT arm (section 4.3: locked cards must not react).
    bool selectCard(PlayerType type);
    int  armedCard() const;                 // -1 when no card is armed

    // tests / frontend helpers
    void seedRandom(unsigned int seed) { m_rng.seed(seed); }

    // --- read-only world state (what the frontend renders) ----------------
    const Playfield&               playfield() const { return m_playfield; }
    const Economy&                 economy() const { return m_economy; }
    const Waves&                   waves() const { return m_waves; }
    const Cutscene&                cutscene() const { return m_cutscene; }
    const Menu&                    menu() const { return m_menu; }
    const std::vector<Player>&     players() const { return m_players; }
    const std::vector<Enemy>&      enemies() const { return m_enemies; }
    const std::vector<Projectile>& projectiles() const { return m_projectiles; }
    const std::vector<Collectible>& collectibles() const { return m_collectibles.items(); }

    int alivePlayerCount() const;
    int aliveEnemyCount() const;
    int projectilesInFlight() const;

    // one line summary - used for the window title and console logging
    std::string statusLine() const;

    // events collected since the previous call (the vector is emptied)
    std::vector<GameEvent> drainEvents();

private:
    // --- state machine ----------------------------------------------------
    void setState(GameState s);
    void startNewGame();        // main menu "Start" -> cutscene, then gameplay
    void restartGameplay();     // pause / win-lose "Restart" -> fresh world
    void returnToMainMenu();
    void resetWorld();

    void updateMainMenu(const InputState& in);
    void updateCutscene(float dt, const InputState& in);
    void updatePlaying(float dt, const InputState& in);
    void updatePaused(const InputState& in);
    void updateEndMenu(const InputState& in);

    MenuAction runMenu(const InputState& in);
    void       handleMenuAction(MenuAction action);

    // --- gameplay steps ---------------------------------------------------
    void handleGridInput(const InputState& in);
    void handleStanceInput(const InputState& in);
    bool tryPlacePlayer(int lane);
    void updateWaves(float dt);
    void updatePlayers(float dt);
    void updateEnemies(float dt);
    void updateProjectiles(float dt);
    void removeDeadCharacters();
    void updateCollectibles(float dt);
    void syncUnlocks();
    void checkEndConditions();

    // --- queries & combat helpers ----------------------------------------
    Enemy*  blockingEnemyFor(const Player& p) const;
    Player* blockingPlayerFor(const Enemy& e) const;
    Enemy*  nearestEnemyInFront(int lane, float x, float maxRange) const;
    Player* nearestPlayerInFront(int lane, float x, float maxRange) const;
    Player* defenseShielding(const Player& p) const;
    bool    hitPlayer(Player& p, int amount, AttackHeight height);
    void    fire(int ownerId, Faction faction, int lane, float x, float vx,
                 AttackHeight height, int damage);

    // --- book keeping -----------------------------------------------------
    void push(SfxId sfx, const std::string& message = std::string());

    GameState m_state;
    bool      m_quitRequested;

    int       m_nextId;
    float     m_time;
    bool      m_cardArmed;
    PlayerType m_armedCard;

    int       m_lastWaveSeen;
    WavePhase m_lastWavePhase;

    Playfield       m_playfield;
    Economy         m_economy;
    Waves           m_waves;
    Collectibles    m_collectibles;
    Cutscene        m_cutscene;
    Menu            m_menu;
    Random          m_rng;
    NullSoundSystem m_sound;

    std::vector<Player>     m_players;
    std::vector<Enemy>      m_enemies;
    std::vector<Projectile> m_projectiles;
    std::vector<GameEvent>  m_events;
};

#endif // GAME_H
