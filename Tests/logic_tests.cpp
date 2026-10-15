// ---------------------------------------------------------------------------
// Tests/logic_tests.cpp
//
// A console harness for the gameplay logic. It drives Game exactly the way
// main.cpp does (one InputState per frame), so everything it proves is the
// real game, not a copy of the rules.
//
// Build (the file has its own main(), so it is NOT part of Project1.vcxproj):
//
//   MSVC : cl /EHsc /std:c++17 /I.. Tests\logic_tests.cpp ^
//            ..\Playfield.cpp ..\Player.cpp ..\Enemy.cpp ..\Projectile.cpp ^
//            ..\Collectibles.cpp ..\Economy.cpp ..\Waves.cpp ..\Menus.cpp ^
//            ..\Cutscene.cpp ..\Game.cpp /Fe:logic_tests.exe
//   g++  : g++ -std=c++17 -I.. Tests/logic_tests.cpp Playfield.cpp Player.cpp \
//            Enemy.cpp Projectile.cpp Collectibles.cpp Economy.cpp Waves.cpp \
//            Menus.cpp Cutscene.cpp Game.cpp -o logic_tests
//
// It exits with 0 when every check passed and 1 otherwise.
// ---------------------------------------------------------------------------

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

#include "../Core/Types.h"
#include "../Core/Input.h"
#include "../Cutscene.h"
#include "../Economy.h"
#include "../Enemy.h"
#include "../Game.h"
#include "../Playfield.h"
#include "../Player.h"
#include "../Waves.h"

// --- tiny assertion helpers -----------------------------------------------

int g_passed = 0;
int g_failed = 0;

void check(bool ok, const std::string& name, const std::string& detail = std::string())
{
    if (ok) {
        ++g_passed;
        std::cout << "  [ ok ] " << name << "\n";
    }
    else {
        ++g_failed;
        std::cout << "  [FAIL] " << name;
        if (!detail.empty()) std::cout << "   (" << detail << ")";
        std::cout << "\n";
    }
}

void checkNear(float value, float expected, float tolerance, const std::string& name)
{
    check(std::fabs(value - expected) <= tolerance, name);
}

const float FRAME = 1.0f / 60.0f;

// runs `seconds` of frames; the edges of `first` are delivered on frame 1 only
void step(Game& game, float seconds, const InputState& first = InputState())
{
    int frames = static_cast<int>(seconds / FRAME);
    InputState in = first;
    for (int i = 0; i < frames; ++i) {
        game.update(FRAME, in);
        in.clearEdges();
    }
}

void stepFrames(Game& game, int frames, const InputState& first = InputState())
{
    InputState in = first;
    for (int i = 0; i < frames; ++i) {
        game.update(FRAME, in);
        in.clearEdges();
    }
}

// a left click in the middle of a lane
InputState clickLane(int lane)
{
    InputState in;
    in.mouseX = 6.0f;
    in.mouseY = static_cast<float>(lane) * LANE_HEIGHT + 0.5f * LANE_HEIGHT;
    in.leftClickPressed = true;
    return in;
}

InputState pressCard(int which)      // 1 = Melee, 2 = Ranged, 3 = Defense
{
    InputState in;
    if (which == 1) in.card1Pressed = true;
    if (which == 2) in.card2Pressed = true;
    if (which == 3) in.card3Pressed = true;
    return in;
}

InputState pressKey(char key)        // 'w' 's' 'e' (escape) 'n' (enter) 'd' (down)
{
    InputState in;
    if (key == 'w') in.wPressed = true;
    if (key == 's') in.sPressed = true;
    if (key == 'e') in.escapePressed = true;
    if (key == 'n') in.enterPressed = true;
    if (key == 'd') in.downPressed = true;
    return in;
}

// main menu -> Enter -> skip the cutscene -> gameplay
void enterPlaying(Game& game, unsigned int seed)
{
    game.seedRandom(seed);
    stepFrames(game, 1, pressKey('n'));     // "Start Game"
    stepFrames(game, 1, pressKey('e'));     // Esc skips the intro
}

// ---------------------------------------------------------------------------
// 1. menus & cutscene
// ---------------------------------------------------------------------------

void testMenuAndCutscene()
{
    std::cout << "\n[1] main menu, cutscene, state machine\n";

    Game game;
    game.seedRandom(1);

    check(game.state() == GameState::MainMenu, "the game starts in the main menu");
    check(game.menu().isOpen() && game.menu().kind() == MenuKind::Main, "the main menu is open");
    check(game.menu().itemCount() == 3, "the main menu has 3 entries");
    check(std::string(game.menu().itemLabel(0)) == "Start Game", "entry 0 is Start Game");
    check(game.menu().selectedIndex() == 0, "the first entry is highlighted");

    stepFrames(game, 1, pressKey('d'));
    check(game.menu().selectedIndex() == 1, "Down moves the highlight");
    stepFrames(game, 1, pressKey('d'));
    check(game.menu().selectedIndex() == 2, "Down again");
    stepFrames(game, 1, pressKey('d'));
    check(game.menu().selectedIndex() == 0, "the highlight wraps around");

    stepFrames(game, 1, pressKey('n'));               // Start Game
    check(game.state() == GameState::Cutscene, "Enter on Start Game runs the cutscene");
    check(game.cutscene().playing(), "the cutscene is playing");

    float duration = game.cutscene().duration();
    check(duration >= 10.0f && duration <= 20.0f, "the cutscene lasts 10-20 seconds");

    int playerActors = 0;
    int enemyActors = 0;
    const std::vector<CutsceneActor>& actors = game.cutscene().actors();
    for (size_t i = 0; i < actors.size(); ++i) {
        if (actors[i].isPlayer) ++playerActors; else ++enemyActors;
    }
    check(playerActors >= 1 && enemyActors >= 1, "the cutscene shows a player and an enemy");

    // the scene must move and speak, not only print text
    step(game, 3.0f);
    bool moved = false;
    for (size_t i = 0; i < actors.size(); ++i) {
        float dx = game.cutscene().actors()[i].pos.x - game.cutscene().actors()[i].startPos.x;
        if (std::fabs(dx) > 0.5f) moved = true;
    }
    check(moved, "cutscene actors are animated (they move while talking)");
    check(!game.cutscene().currentSubtitle().empty(), "dialogue is shown during the scene");

    step(game, duration + 1.0f);
    check(game.state() == GameState::Playing, "when the cutscene ends the game starts");
    check(game.players().size() == 0, "gameplay starts with an empty field");
    check(game.economy().coins() == STARTING_COINS, "gameplay starts with 30 coins");
    check(game.waves().currentWave() == 0, "wave 1 is the running wave");

    step(game, 1.0f);
    check(game.enemies().size() > 0, "wave 1 spawns its first enemy right away");
}

// ---------------------------------------------------------------------------
// 2. lanes, placement, cost, locked cards
// ---------------------------------------------------------------------------

void testLanesAndPlacement()
{
    std::cout << "\n[2] lane selection, player placement, locked cards\n";

    Game game;
    enterPlaying(game, 7);
    check(game.state() == GameState::Playing, "the game is running");

    // hovering and clicking
    InputState hover;
    hover.mouseY = 1.0f * LANE_HEIGHT + 0.2f;
    stepFrames(game, 1, hover);
    check(game.playfield().hoveredLane() == 1, "the hovered lane follows the cursor");
    check(!game.playfield().isLaneSelected(1), "hovering alone does not select");

    stepFrames(game, 1, clickLane(1));
    check(game.playfield().isLaneSelected(1), "a click selects the lane");
    check(game.playfield().hoveredLane() == 1, "the selected lane stays highlighted");

    // a second click deselects
    stepFrames(game, 1, clickLane(1));
    check(!game.playfield().isLaneSelected(1), "a second click deselects the lane");

    // place a melee player on the selected lane
    stepFrames(game, 1, clickLane(1));
    stepFrames(game, 1, pressCard(1));

    check(game.players().size() == 1, "the melee card placed a player");
    check(game.players()[0].type == PlayerType::Melee, "it is a melee player");
    check(game.players()[0].lane == 1, "it spawned on the selected lane");
    checkNear(game.players()[0].x, game.playfield().leftmostX(),
              PLAYER_ADVANCE_SPEED * FRAME + 1e-4f,
              "it spawned at the leftmost column of that lane");
    check(game.economy().coins() == STARTING_COINS - MELEE_COST, "the melee cost was paid");
    check(game.armedCard() == -1, "the card is disarmed after placing");

    // locked cards must not react at all
    check(!game.economy().isUnlocked(PlayerType::Ranged), "the ranged card is locked at the start");
    check(!game.economy().isUnlocked(PlayerType::Defense), "the defense card is locked at the start");
    stepFrames(game, 1, pressCard(2));
    stepFrames(game, 1, pressCard(3));
    check(game.players().size() == 1, "locked cards place nothing");
    check(game.armedCard() == -1, "locked cards never arm");
    check(game.economy().coins() == STARTING_COINS - MELEE_COST, "locked cards cost nothing");

    // a player never switches lanes
    int lane = game.players()[0].lane;
    step(game, 3.0f);
    check(game.players().size() == 1 && game.players()[0].lane == lane,
          "a player stays on its lane while advancing");
    check(game.players()[0].x > game.playfield().leftmostX(), "players advance to the right");

    // placement can also happen the other way round: arm, then click a lane
    Game second;
    enterPlaying(second, 11);
    stepFrames(second, 1, pressCard(1));
    check(second.players().size() == 0 && second.armedCard() == static_cast<int>(PlayerType::Melee),
          "with no lane selected the card waits to be armed");
    stepFrames(second, 1, clickLane(2));
    check(second.players().size() == 1 && second.players()[0].lane == 2,
          "clicking a lane places the armed card");
    check(second.armedCard() == -1, "the armed card is consumed");
}

// ---------------------------------------------------------------------------
// 3. jump & duck (pure logic)
// ---------------------------------------------------------------------------

void testJumpAndDuck()
{
    std::cout << "\n[3] jumping and ducking (section 3.8)\n";

    Playfield selected;
    selected.toggleLaneSelected(0);
    selected.toggleLaneSelected(2);

    std::vector<Player> players;
    players.push_back(makePlayer(1, PlayerType::Melee, 0, 5.0f));
    players.push_back(makePlayer(2, PlayerType::Ranged, 1, 5.0f));   // lane NOT selected
    players.push_back(makePlayer(3, PlayerType::Defense, 0, 5.0f));  // defense never reacts

    check(commandJump(players, selected) == 1, "jump only affects the selected lanes");
    check(players[0].isJumping(), "the selected player jumps");
    check(!players[1].isJumping(), "a player on an unselected lane does not jump");
    check(!players[2].isJumping(), "the defense player never jumps");

    check(players[0].dodges(AttackHeight::Low), "a jumping player dodges LOW attacks");
    check(!players[0].dodges(AttackHeight::High), "a jumping player does not dodge HIGH attacks");
    check(!players[0].dodges(AttackHeight::Straight), "a jump never dodges a straight hit");
    check(!players[2].dodges(AttackHeight::Low), "the defense player cannot dodge");

    // ducking
    check(commandDuck(players, selected) == 1, "duck only affects the selected lanes");
    check(players[0].isDucking() && !players[0].isJumping(), "a duck cancels a jump");
    check(players[0].dodges(AttackHeight::High), "a ducking player dodges HIGH attacks");
    check(!players[0].dodges(AttackHeight::Low), "a ducking player does not dodge LOW attacks");

    // the stance expires
    for (int i = 0; i < 60; ++i) players[0].tickPlayer(FRAME);
    check(!players[0].isDucking(), "a duck lasts only while its timer runs");

    // no lane selected -> nothing happens
    Playfield empty;
    std::vector<Player> lonely;
    lonely.push_back(makePlayer(1, PlayerType::Melee, 0, 5.0f));
    check(commandJump(lonely, empty) == 0, "jump does nothing without a selected lane");
    check(commandDuck(lonely, empty) == 0, "duck does nothing without a selected lane");
    check(!lonely[0].isJumping() && !lonely[0].isDucking(), "and the player stays grounded");
}

// ---------------------------------------------------------------------------
// 4. defense protection (pure logic)
// ---------------------------------------------------------------------------

void testDefenseProtection()
{
    std::cout << "\n[4] the defense player as a shield (section 3.2)\n";

    std::vector<Player> group;
    group.push_back(makePlayer(1, PlayerType::Defense, 0, 3.0f));   // the shield
    group.push_back(makePlayer(2, PlayerType::Melee,   0, 2.5f));   // behind it
    group.push_back(makePlayer(3, PlayerType::Melee,   0, 5.0f));   // ahead of it
    group.push_back(makePlayer(4, PlayerType::Melee,   1, 2.5f));   // other lane

    check(findDefenseShield(group, group[1]) == &group[0],
          "a player behind the defense player is protected");
    check(findDefenseShield(group, group[2]) == 0,
          "once the player moves past the defense player it is vulnerable");
    check(findDefenseShield(group, group[0]) == 0,
          "the defense player is not shielded by itself");
    check(findDefenseShield(group, group[3]) == 0, "protection works per lane");

    check(DEFENSE_HP == 7, "the defense player has 7 lives");
}

// ---------------------------------------------------------------------------
// 5. economy: costs, unlocking, re-locking (pure logic)
// ---------------------------------------------------------------------------

void testEconomy()
{
    std::cout << "\n[5] coins, unlocking and re-locking (sections 3.2 / 4.3)\n";

    Economy e;
    check(e.coins() == STARTING_COINS, "the game starts with 30 coins");
    check(e.earnedTotal() == 0, "nothing has been collected yet");
    check(e.isUnlocked(PlayerType::Melee), "melee is unlocked from the start");
    check(!e.isUnlocked(PlayerType::Ranged), "ranged is locked from the start");
    check(!e.isUnlocked(PlayerType::Defense), "defense is locked from the start");

    check(Economy::costOf(PlayerType::Ranged) == 2 * Economy::costOf(PlayerType::Melee),
          "ranged costs twice the melee card");
    check(Economy::costOf(PlayerType::Defense) == 3 * Economy::costOf(PlayerType::Melee),
          "defense costs three times the melee card");
    check(e.canAfford(PlayerType::Melee), "30 coins afford the melee card");

    check(e.spend(PlayerType::Melee) && e.coins() == STARTING_COINS - MELEE_COST,
          "buying melee costs 10");
    check(e.spend(PlayerType::Melee), "melee can be bought again");
    check(e.coins() == STARTING_COINS - 2 * MELEE_COST, "the starting coins allow 3 melee players");
    check(!e.spend(PlayerType::Defense), "a locked card cannot be bought");

    e.earn(20);
    check(e.coins() == 30 && e.earnedTotal() == 20, "collecting 20 points is tracked");
    check(e.isUnlocked(PlayerType::Ranged), "ranged unlocks after 20 collected points");
    check(!e.isUnlocked(PlayerType::Defense), "defense still needs 30 collected points");

    bool announcedUnlock = false;
    for (;;) {
        UnlockChange change = e.refreshUnlocks();
        if (change == UnlockChange::None) break;
        if (change == UnlockChange::RangedUnlocked) announcedUnlock = true;
    }
    check(announcedUnlock, "the unlock transition is reported (so it can play a sound)");

    e.earn(10);
    check(e.isUnlocked(PlayerType::Defense), "defense unlocks after 30 collected points");

    check(e.spend(PlayerType::Ranged) && e.spend(PlayerType::Ranged), "the ranged card can be bought twice");
    check(e.coins() == 0, "the coins are gone");
    check(!e.isUnlocked(PlayerType::Ranged), "falling below the cost re-locks the card");

    bool announcedRelock = false;
    for (;;) {
        UnlockChange change = e.refreshUnlocks();
        if (change == UnlockChange::None) break;
        if (change == UnlockChange::RangedRelocked) announcedRelock = true;
    }
    check(announcedRelock, "the re-lock transition is reported as well");
}

// ---------------------------------------------------------------------------
// 6. waves (pure logic, simulated to the very end)
// ---------------------------------------------------------------------------

void testWaves()
{
    std::cout << "\n[6] the three waves (section 3.3)\n";

    Waves waves;
    Random rng;
    rng.seed(99);

    std::vector<Enemy> enemies;
    int nextId = 1;

    int spawnedPerWave[3] = { 0, 0, 0 };
    bool sawEasy[3] = { false, false, false };
    bool sawMedium[3] = { false, false, false };
    bool sawHard[3] = { false, false, false };
    bool lanesValid = true;
    int totalSpawned = 0;

    for (int frame = 0; frame < 60 * 300; ++frame) {
        size_t before = enemies.size();
        waves.update(0.1f, enemies, nextId, rng);

        if (enemies.size() > before) {
            for (size_t i = before; i < enemies.size(); ++i) {
                int wave = waves.currentWave();
                if (wave >= 0 && wave < 3) {
                    ++spawnedPerWave[wave];
                    if (enemies[i].type == EnemyType::EasyUpper)   sawEasy[wave] = true;
                    if (enemies[i].type == EnemyType::MediumLower) sawMedium[wave] = true;
                    if (enemies[i].type == EnemyType::HardMelee)   sawHard[wave] = true;
                }
                if (enemies[i].lane < 0 || enemies[i].lane >= LANE_COUNT) lanesValid = false;
                if (std::fabs(enemies[i].x - ENEMY_SPAWN_X) > 1e-4f) lanesValid = false;
                ++totalSpawned;
            }
        }

        // pretend every enemy is killed instantly so the waves can advance
        enemies.clear();

        if (waves.allCleared(enemies)) break;
    }

    check(totalSpawned >= WAVE1_MIN_ENEMIES + WAVE2_MIN_ENEMIES + WAVE3_MIN_ENEMIES,
          "every wave produced at least its minimum number of enemies");
    check(spawnedPerWave[0] >= WAVE1_MIN_ENEMIES, "wave 1 has at least 3 enemies");
    check(spawnedPerWave[1] >= WAVE2_MIN_ENEMIES, "wave 2 has at least 5 enemies");
    check(spawnedPerWave[2] >= WAVE3_MIN_ENEMIES, "wave 3 has at least 10 enemies");
    check(sawEasy[0] && !sawMedium[0] && !sawHard[0], "wave 1 is easy enemies only");
    check(sawEasy[1] && sawMedium[1] && !sawHard[1], "wave 2 mixes easy and medium");
    check(sawEasy[2] && sawMedium[2] && sawHard[2], "wave 3 mixes all three types");
    check(lanesValid, "every enemy spawns at the right edge of a valid lane");
    check(waves.allCleared(enemies), "after wave 3 the game is won (allCleared)");
}

// ---------------------------------------------------------------------------
// 7. combat, collectibles, unlocking and losing (integration)
// ---------------------------------------------------------------------------

void testCombatAndLosing()
{
    std::cout << "\n[7] combat, collectibles and losing (integration)\n";

    Game game;
    enterPlaying(game, 7);          // a fixed seed keeps this deterministic

    // three melee players, one per lane - exactly the 30 starting coins
    for (int lane = 0; lane < 3; ++lane) {
        stepFrames(game, 1, clickLane(lane));
        stepFrames(game, 1, pressCard(1));
    }
    check(game.players().size() == 3, "three melee players were placed");
    check(game.economy().coins() == 0, "all 30 starting coins were spent");
    check(game.economy().earnedTotal() == 0, "no coins were collected yet");

    int maxEnemies = 0;
    bool sawEnemyDied = false;
    bool sawCollectible = false;

    for (int frame = 0; frame < 60 * 40; ++frame) {
        game.update(FRAME, InputState());

        std::vector<GameEvent> events = game.drainEvents();
        for (size_t i = 0; i < events.size(); ++i) {
            if (events[i].sfx == SfxId::EnemyDied)    sawEnemyDied = true;
            if (events[i].sfx == SfxId::CollectiblePicked) sawCollectible = true;
        }

        if (game.aliveEnemyCount() > maxEnemies) maxEnemies = game.aliveEnemyCount();
        if (game.state() != GameState::Playing) break;
    }

    check(maxEnemies > 0, "enemies spawned and advanced into the field");
    check(sawEnemyDied, "a melee player defeated an enemy");
    check(sawCollectible, "the defeated enemy dropped a collectible that was picked up");
    check(game.economy().earnedTotal() > 0, "collecting coins raises the earned total");

    // the fourth lane has no defender, so an enemy should reach the home base
    check(game.state() == GameState::Lost, "an unopposed enemy reaches the home base -> Lost");
    check(game.menu().kind() == MenuKind::WinLose, "the win/lose menu is offered");

    std::vector<float> frozenPositions;
    for (size_t i = 0; i < game.enemies().size(); ++i)
        frozenPositions.push_back(game.enemies()[i].x);

    step(game, 2.0f);

    bool stillFrozen = (game.enemies().size() == frozenPositions.size());
    for (size_t i = 0; i < game.enemies().size() && i < frozenPositions.size(); ++i)
        if (std::fabs(game.enemies()[i].x - frozenPositions[i]) > 1e-6f) stillFrozen = false;
    check(stillFrozen, "everything stops once the game is lost");
}

// ---------------------------------------------------------------------------
// 8. pause menu, resume, restart
// ---------------------------------------------------------------------------

void testPauseMenu()
{
    std::cout << "\n[8] pause menu (section 4.3)\n";

    Game game;
    enterPlaying(game, 3);
    stepFrames(game, 1, clickLane(0));
    stepFrames(game, 1, pressCard(1));

    float xBefore = game.players()[0].x;
    stepFrames(game, 1, pressKey('e'));
    check(game.state() == GameState::Paused, "Esc pauses the game");
    check(game.menu().isOpen() && game.menu().kind() == MenuKind::Pause, "the pause menu opened");
    check(game.menu().itemCount() == 3, "the pause menu has 3 entries");
    check(std::string(game.menu().itemLabel(0)) == "Continue", "entry 0 is Continue");

    step(game, 2.0f);
    checkNear(game.players()[0].x, xBefore, 1e-4f, "the battlefield is frozen while paused");

    stepFrames(game, 1, pressKey('n'));                 // Continue
    check(game.state() == GameState::Playing, "Continue resumes the game");
    check(!game.menu().isOpen(), "the pause menu closed");

    // resume with Esc as well
    stepFrames(game, 1, pressKey('e'));
    check(game.state() == GameState::Paused, "Esc pauses again");
    stepFrames(game, 1, pressKey('e'));
    check(game.state() == GameState::Playing, "Esc resumes as well");

    // restart from the pause menu
    stepFrames(game, 1, pressKey('e'));
    stepFrames(game, 1, pressKey('d'));                 // Restart
    check(game.menu().selectedIndex() == 1, "Down highlights Restart");
    stepFrames(game, 1, pressKey('n'));
    check(game.state() == GameState::Playing, "Restart goes back into the game");
    check(game.players().size() == 0, "Restart clears the battlefield");
    check(game.economy().coins() == STARTING_COINS, "Restart restores the coins");
    stepFrames(game, 1, InputState());                  // one gameplay frame
    check(game.waves().currentWave() == 0, "Restart starts at wave 1 again");
    check(game.enemies().size() == 1, "wave 1 spawns its first enemy again");

    // and back to the main menu
    stepFrames(game, 1, pressKey('e'));
    stepFrames(game, 1, pressKey('d'));
    stepFrames(game, 1, pressKey('d'));                 // Return to Main Menu
    check(std::string(game.menu().itemLabel(game.menu().selectedIndex())) == "Return to Main Menu",
          "the third entry returns to the main menu");
    stepFrames(game, 1, pressKey('n'));
    check(game.state() == GameState::MainMenu, "Return to Main Menu works");
    check(game.menu().kind() == MenuKind::Main, "the main menu is shown again");
}

// ---------------------------------------------------------------------------
// 9. jumping and ducking through the real input path
// ---------------------------------------------------------------------------

void testStanceThroughGame()
{
    std::cout << "\n[9] jump / duck through the game input\n";

    Game game;
    enterPlaying(game, 5);
    stepFrames(game, 1, clickLane(0));
    stepFrames(game, 1, pressCard(1));
    check(game.players().size() == 1, "a melee player was placed");

    stepFrames(game, 1, pressKey('w'));
    check(game.players()[0].isJumping(), "W makes the selected lane's players jump");

    stepFrames(game, 1, pressKey('s'));
    check(game.players()[0].isDucking(), "S makes them duck");
    check(!game.players()[0].isJumping(), "ducking cancels the jump");

    // a lane that is not selected and has no player is not affected either
    stepFrames(game, 1, clickLane(3));
    check(game.playfield().isLaneSelected(3), "lane 3 is selected too");
    check(game.playfield().selectedLaneCount() == 2, "two lanes are selected at once");
    stepFrames(game, 1, clickLane(3));
    check(game.playfield().selectedLaneCount() == 1, "deselecting leaves one selection");
}

// ---------------------------------------------------------------------------
// 10. everyone attacks as soon as it exists, target or not (page 4)
// ---------------------------------------------------------------------------

void testAttacksWithoutTargets()
{
    std::cout << "\n[10] attacking with nothing in front (page 4)\n";

    Game game;
    enterPlaying(game, 7);
    stepFrames(game, 1);        // the first gameplay frame spawns wave 1

    check(!game.enemies().empty(), "an enemy can be on the grid before any player");
    if (!game.enemies().empty()) {
        check(game.enemies()[0].attacking, "it is attacking although no player was placed");
        check(game.enemies()[0].state == CharState::Advancing, "and it walks while it attacks");
    }

    // ranged enemies fire with an empty grid - the shots just fly away
    bool enemyShotSeen = false;
    for (int i = 0; i < 60 * 4 && !enemyShotSeen; ++i) {
        stepFrames(game, 1);
        const std::vector<Projectile>& shots = game.projectiles();
        for (size_t s = 0; s < shots.size(); ++s)
            if (shots[s].active && shots[s].faction == Faction::Enemy) enemyShotSeen = true;
    }
    check(enemyShotSeen, "a ranged enemy fires although there is no player on the grid");

    // the melee player swings from the frame it is placed on
    stepFrames(game, 1, clickLane(2));
    stepFrames(game, 1, pressCard(1));
    check(game.players().size() == 1, "a melee player was placed");
    if (!game.players().empty())
        check(game.players()[0].attacking, "it is swinging straight away");

    // nothing in front of it: the swings must not wound anybody
    // a shot that hits nobody dies after its range, so a stray attack can never
    // reach a character standing on the other side of the playfield
    bool withinRange = true;
    for (int i = 0; i < 60 * 12; ++i) {
        stepFrames(game, 1);
        const std::vector<Projectile>& shots = game.projectiles();
        for (size_t s = 0; s < shots.size(); ++s)
            if (shots[s].active && shots[s].distanceTravelled() > shots[s].range) withinRange = false;
        if (game.state() != GameState::Playing) break;
    }
    check(withinRange, "no shot ever travels farther than the weapon reaches");
}

// ---------------------------------------------------------------------------

int main()
{
    std::cout << "Project1 - gameplay logic tests\n";

    testMenuAndCutscene();
    testLanesAndPlacement();
    testJumpAndDuck();
    testDefenseProtection();
    testEconomy();
    testWaves();
    testCombatAndLosing();
    testPauseMenu();
    testStanceThroughGame();
    testAttacksWithoutTargets();

    std::cout << "\n----------------------------------------\n";
    std::cout << "passed: " << g_passed << "   failed: " << g_failed << "\n";

    return (g_failed == 0) ? 0 : 1;
}
