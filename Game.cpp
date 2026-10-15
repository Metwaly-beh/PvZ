#include "Game.h"

#include <cmath>
#include <cstdio>
#include <iostream>

namespace {

std::string laneText(int lane)
{
    std::string s = "lane ";
    s += static_cast<char>('0' + lane);
    return s;
}

} // namespace

Game::Game()
    : m_state(GameState::MainMenu),
      m_quitRequested(false),
      m_nextId(1),
      m_time(0.0f),
      m_cardArmed(false),
      m_armedCard(PlayerType::Melee),
      m_lastWaveSeen(-1),
      m_lastWavePhase(WavePhase::NotStarted)
{
    resetWorld();
    m_menu.open(MenuKind::Main);     // the game launches into the main menu
}

// ---------------------------------------------------------------------------
// top level
// ---------------------------------------------------------------------------

void Game::update(float dt, const InputState& in)
{
    m_time += dt;

    switch (m_state) {
    case GameState::MainMenu: updateMainMenu(in);         break;
    case GameState::Cutscene: updateCutscene(dt, in);     break;
    case GameState::Playing:  updatePlaying(dt, in);      break;
    case GameState::Paused:   updatePaused(in);           break;
    case GameState::Won:
    case GameState::Lost:     updateEndMenu(in);          break;
    }
}

void Game::setState(GameState s)
{
    if (m_state == s) return;
    m_state = s;
    std::cout << "[game] state -> " << toString(s) << "\n";
}

void Game::resetWorld()
{
    m_players.clear();
    m_enemies.clear();
    m_projectiles.clear();
    m_collectibles.clear();

    m_economy.reset();
    m_waves.reset();
    m_playfield.clearSelection();
    m_playfield.setHoveredLane(-1);

    m_cardArmed = false;
    m_armedCard = PlayerType::Melee;
    m_nextId = 1;
    m_lastWaveSeen = -1;
    m_lastWavePhase = WavePhase::NotStarted;

    m_cutscene = Cutscene();
}

void Game::startNewGame()
{
    resetWorld();
    m_menu.close();
    m_cutscene.start();
    m_sound.playBackgroundMusic();
    setState(GameState::Cutscene);
    push(SfxId::CutsceneStarted, "intro cutscene");
}

void Game::restartGameplay()
{
    // "Restart the game" from the pause / win-lose menu: a fresh battlefield,
    // straight into gameplay (the cutscene only plays from the main menu).
    resetWorld();
    m_menu.close();
    m_sound.playBackgroundMusic();
    setState(GameState::Playing);
    push(SfxId::MenuConfirmed, "restart");
}

void Game::returnToMainMenu()
{
    resetWorld();
    m_sound.stopBackgroundMusic();
    m_menu.open(MenuKind::Main);
    setState(GameState::MainMenu);
}

// ---------------------------------------------------------------------------
// menu / cutscene states
// ---------------------------------------------------------------------------

MenuAction Game::runMenu(const InputState& in)
{
    int before = m_menu.selectedIndex();
    MenuAction action = m_menu.handleInput(in);

    if (m_menu.selectedIndex() != before)
        push(SfxId::MenuMoved, m_menu.itemLabel(m_menu.selectedIndex()));

    if (action != MenuAction::None)
        push(SfxId::MenuConfirmed, m_menu.itemLabel(m_menu.selectedIndex()));

    return action;
}

void Game::handleMenuAction(MenuAction action)
{
    switch (action) {
    case MenuAction::Start:
        startNewGame();
        break;

    case MenuAction::Resume:
        m_menu.close();
        setState(GameState::Playing);
        break;

    case MenuAction::Restart:
        restartGameplay();
        break;

    case MenuAction::GoToMainMenu:
        returnToMainMenu();
        break;

    case MenuAction::Exit:
        m_quitRequested = true;
        break;

    case MenuAction::OpenSoundSettings:      // bonus
        m_menu.open(MenuKind::SoundSettings);
        break;

    case MenuAction::Back:                   // bonus
        m_menu.open(MenuKind::Main);
        break;

    case MenuAction::ToggleMute:
        m_sound.toggleMute();
        break;

    case MenuAction::VolumeUp:
        m_sound.volumeUp();
        break;

    case MenuAction::VolumeDown:
        m_sound.volumeDown();
        break;

    case MenuAction::None:
        break;
    }
}

void Game::updateMainMenu(const InputState& in)
{
    MenuAction action = runMenu(in);
    if (action != MenuAction::None) handleMenuAction(action);
}

void Game::updateCutscene(float dt, const InputState& in)
{
    // Esc skips the intro (handy while testing - the full scene still plays
    // when nobody presses it).
    if (!in.escapePressed) {
        m_cutscene.update(dt);
        if (!m_cutscene.finished()) return;
    }

    push(SfxId::CutsceneFinished, "starting gameplay");
    resetWorld();                       // the world starts when the scene ends
    setState(GameState::Playing);
}

void Game::updatePaused(const InputState& in)
{
    if (in.escapePressed) {             // Esc toggles the pause menu
        m_menu.close();
        setState(GameState::Playing);
        return;
    }

    MenuAction action = runMenu(in);
    if (action != MenuAction::None) handleMenuAction(action);
}

void Game::updateEndMenu(const InputState& in)
{
    MenuAction action = runMenu(in);
    if (action != MenuAction::None) handleMenuAction(action);
}

// ---------------------------------------------------------------------------
// the actual game
// ---------------------------------------------------------------------------

void Game::updatePlaying(float dt, const InputState& in)
{
    // Esc opens the pause menu; the world below is not updated at all while
    // paused, which is exactly the "everything stops" behaviour of section 3.4
    if (in.escapePressed) {
        m_menu.open(MenuKind::Pause);
        setState(GameState::Paused);
        return;
    }

    handleGridInput(in);
    handleStanceInput(in);

    updateWaves(dt);
    updatePlayers(dt);
    updateEnemies(dt);
    updateProjectiles(dt);

    removeDeadCharacters();
    updateCollectibles(dt);
    syncUnlocks();

    checkEndConditions();
}

// --- input ----------------------------------------------------------------

void Game::handleGridInput(const InputState& in)
{
    // the cursor is over the grid -> that lane is highlighted by the frontend
    m_playfield.setHoveredLane(m_playfield.laneAtPoint(in.mouseX, in.mouseY));

    // the harness / a future UI can also arm cards with the number keys
    if (in.card1Pressed) selectCard(PlayerType::Melee);
    if (in.card2Pressed) selectCard(PlayerType::Ranged);
    if (in.card3Pressed) selectCard(PlayerType::Defense);

    if (!in.leftClickPressed) return;

    int hovered = m_playfield.hoveredLane();
    if (hovered < 0) return;            // clicked somewhere off the lanes

    if (m_cardArmed) {
        // an armed card turns the click into a placement
        if (!m_playfield.isLaneSelected(hovered))
            m_playfield.toggleLaneSelected(hovered);
        tryPlacePlayer(hovered);
    }
    else {
        // plain click: select / deselect the whole lane
        bool selected = m_playfield.toggleLaneSelected(hovered);
        push(selected ? SfxId::LaneSelected : SfxId::LaneDeselected,
             laneText(hovered));
    }
}

void Game::handleStanceInput(const InputState& in)
{
    if (in.jumpPressed()) {
        int told = commandJump(m_players, m_playfield);
        if (told > 0) push(SfxId::PlayerJump, "jump");
        else          push(SfxId::NoLaneSelected, "jump: no lane selected");
    }

    if (in.duckPressed()) {
        int told = commandDuck(m_players, m_playfield);
        if (told > 0) push(SfxId::PlayerDuck, "duck");
        else          push(SfxId::NoLaneSelected, "duck: no lane selected");
    }
}

bool Game::selectCard(PlayerType type)
{
    // section 4.3: a locked card must not respond to interaction at all
    if (!m_economy.isUnlocked(type)) {
        push(SfxId::CardLocked, std::string("locked: ") + toString(type));
        return false;
    }
    if (!m_economy.canAfford(type)) {
        push(SfxId::NotEnoughCoins, std::string("cannot afford: ") + toString(type));
        return false;
    }

    m_cardArmed = true;
    m_armedCard = type;
    push(SfxId::CardArmed, std::string("armed: ") + toString(type));

    // "The player then selects a character ... and once the lane is selected,
    //  the player automatically appears at the leftmost position"
    int lane = m_playfield.lastSelectedLane();
    if (lane >= 0) tryPlacePlayer(lane);

    return true;
}

bool Game::tryPlacePlayer(int lane)
{
    if (!m_cardArmed) return false;
    if (lane < 0 || lane >= LANE_COUNT) return false;

    if (!m_economy.isUnlocked(m_armedCard)) {
        m_cardArmed = false;
        push(SfxId::CardLocked, "card got locked");
        return false;
    }
    if (!m_economy.canAfford(m_armedCard)) {
        push(SfxId::NotEnoughCoins, "not enough coins to place");
        return false;
    }

    // players are paid for first, then appear at the leftmost column
    m_economy.spend(m_armedCard);
    m_players.push_back(makePlayer(m_nextId++, m_armedCard, lane, m_playfield.leftmostX()));
    m_playfield.setHoveredLane(lane);

    push(SfxId::PlayerPlaced, std::string(toString(m_armedCard)) + " -> " + laneText(lane));

    m_cardArmed = false;               // one card selection = one placement
    return true;
}

// --- simulation -----------------------------------------------------------

void Game::updateWaves(float dt)
{
    int spawned = m_waves.update(dt, m_enemies, m_nextId, m_rng);

    if (spawned > 0 && m_waves.currentWave() != m_lastWaveSeen) {
        m_lastWaveSeen = m_waves.currentWave();
        char buffer[64];
        std::snprintf(buffer, sizeof(buffer), "wave %d/%d",
                      m_waves.currentWave() + 1, m_waves.waveCount());
        push(SfxId::WaveStarted, buffer);
    }

    if (m_lastWavePhase != WavePhase::Clearing && m_waves.phase() == WavePhase::Clearing) {
        push(SfxId::WaveCleared, "no more spawns - clear the screen");
    }
    m_lastWavePhase = m_waves.phase();
}

void Game::updatePlayers(float dt)
{
    for (size_t i = 0; i < m_players.size(); ++i) {
        Player& p = m_players[i];
        p.tickPlayer(dt);
        if (!p.alive()) continue;

        Enemy* blocker = blockingEnemyFor(p);

        // page 4 of the description: players "immediately start attacking ...
        // and move towards the right side ... as soon as they are placed, even
        // if there are no enemies". So the Melee player keeps swinging and the
        // Ranged player keeps firing on their own rhythm from the moment they
        // exist - a swing or a shot with nothing in front of it simply misses.
        p.attacking = (p.type != PlayerType::Defense);

        if (p.type == PlayerType::Melee) {
            if (p.attackCooldown <= 0.0f) {
                Enemy* target = nearestEnemyInFront(p.lane, p.x, MELEE_RANGE);
                if (target) target->receiveDamage(PLAYER_DAMAGE);
                p.attackCooldown = PLAYER_ATTACK_INTERVAL;
                push(SfxId::PlayerAttack, target ? "melee swing" : "melee swing (air)");
            }
        }
        else if (p.type == PlayerType::Ranged) {
            if (p.attackCooldown <= 0.0f) {
                Enemy* target = nearestEnemyInFront(p.lane, p.x, RANGED_RANGE);
                // the shot is fired either way; without a target it just flies
                // off the right edge of the playfield
                fire(p.id, Faction::Player, p.lane, p.x, PROJECTILE_SPEED,
                     AttackHeight::Straight, PLAYER_DAMAGE);
                p.attackCooldown = PLAYER_ATTACK_INTERVAL;
                push(SfxId::PlayerAttack, target ? "ranged shot" : "ranged shot (air)");
            }
        }
        // the Defense player has NO attack - it only blocks and absorbs

        if (blocker) {
            p.state = CharState::Fighting;      // "both should stop moving"
        }
        else if (p.canMove()) {
            advancePlayer(p, dt, PLAYFIELD_RIGHT);
        }
        else {
            p.state = CharState::Idle;          // defense never moves
        }

        // stopped on purpose (e.g. reached the right edge) but still swinging
        if (p.attacking && p.state != CharState::Advancing &&
            p.state != CharState::Fighting && p.state != CharState::Jumping &&
            p.state != CharState::Ducking) {
            p.state = CharState::Attacking;
        }
    }
}

void Game::updateEnemies(float dt)
{
    for (size_t i = 0; i < m_enemies.size(); ++i) {
        Enemy& e = m_enemies[i];
        e.tickEnemy(dt);
        if (!e.alive()) continue;

        Player* blocker = blockingPlayerFor(e);

        // same rule as the players: enemies attack on their own rhythm from
        // the moment they appear, "even if there are no players" (page 4)
        e.attacking = true;

        if (e.isRanged()) {
            // "attacks the player from a distance"
            if (e.attackCooldown <= 0.0f) {
                Player* target = nearestPlayerInFront(e.lane, e.x, RANGED_RANGE);
                fire(e.id, Faction::Enemy, e.lane, e.x, -PROJECTILE_SPEED,
                     e.attackHeight, enemyDamage(e.type));
                e.attackCooldown = ENEMY_ATTACK_INTERVAL;
                push(SfxId::EnemyAttack, std::string(toString(e.type)) +
                     (target ? " shot" : " shot (air)"));
            }
        }
        else {
            // hard melee: "runs toward the player and attacks at close range"
            if (e.attackCooldown <= 0.0f) {
                Player* target = nearestPlayerInFront(e.lane, e.x, MELEE_RANGE);
                if (target) hitPlayer(*target, enemyDamage(e.type), AttackHeight::Straight);
                e.attackCooldown = ENEMY_ATTACK_INTERVAL;
                push(SfxId::EnemyAttack, "melee swing");
            }
        }

        if (blocker) {
            e.state = CharState::Fighting;
        }
        else {
            // enemies never stop until they reach the home base - they attack
            // while they walk
            e.x -= e.speed * dt;
            if (e.x < 0.0f) e.x = 0.0f;     // safety, the loss check fires earlier
            e.state = CharState::Advancing;
        }
    }
}

void Game::fire(int ownerId, Faction faction, int lane, float x, float vx,
                AttackHeight height, int damage)
{
    m_projectiles.push_back(makeProjectile(m_nextId++, faction, lane, x,
                                           m_playfield.laneCenterY(lane),
                                           vx, height, damage, ownerId));
}

void Game::updateProjectiles(float dt)
{
    for (size_t i = 0; i < m_projectiles.size(); ++i) {
        Projectile& pr = m_projectiles[i];
        if (!pr.active) continue;

        pr.x += pr.vx * dt;
        if (pr.x < 0.0f || pr.x > PLAYFIELD_WIDTH) {
            pr.active = false;
            continue;
        }

        // a shot that hit nothing expires once it has flown its full range
        if (pr.distanceTravelled() > pr.range) {
            pr.active = false;
            continue;
        }

        if (pr.faction == Faction::Player) {
            // player shots only ever hurt ENEMIES - "never harms other players"
            for (size_t j = 0; j < m_enemies.size(); ++j) {
                Enemy& e = m_enemies[j];
                if (!e.alive() || e.lane != pr.lane) continue;
                if (std::fabs(e.x - pr.x) <= PROJECTILE_HIT_RADIUS) {
                    e.receiveDamage(pr.damage);
                    pr.active = false;
                    push(SfxId::ProjectileHit, "shot hit an enemy");
                    break;
                }
            }
        }
        else {
            for (size_t j = 0; j < m_players.size(); ++j) {
                Player& p = m_players[j];
                if (!p.alive() || p.lane != pr.lane) continue;
                if (std::fabs(p.x - pr.x) > PROJECTILE_HIT_RADIUS) continue;

                // a dodged shot simply flies on (and may still hit a player
                // further back, e.g. one that a Defense player protects)
                if (p.dodges(pr.height)) continue;

                hitPlayer(p, pr.damage, pr.height);
                pr.active = false;
                push(SfxId::ProjectileHit, "shot hit a player");
                break;
            }
        }
    }

    // compact
    std::vector<Projectile> kept;
    for (size_t i = 0; i < m_projectiles.size(); ++i)
        if (m_projectiles[i].active) kept.push_back(m_projectiles[i]);
    m_projectiles.swap(kept);
}

void Game::removeDeadCharacters()
{
    // enemies: "Defeated enemies drop collectibles."
    std::vector<Enemy> livingEnemies;
    for (size_t i = 0; i < m_enemies.size(); ++i) {
        Enemy& e = m_enemies[i];
        if (e.alive()) {
            livingEnemies.push_back(e);
        }
        else {
            m_collectibles.drop(m_nextId++, e.lane, e.x, enemyCoinDrop(e.type));
            push(SfxId::EnemyDied, std::string(toString(e.type)) + " defeated");
        }
    }
    m_enemies.swap(livingEnemies);

    // players: "When a player is defeated, it must be removed from the scene."
    std::vector<Player> livingPlayers;
    for (size_t i = 0; i < m_players.size(); ++i) {
        Player& p = m_players[i];
        if (p.alive()) livingPlayers.push_back(p);
        else           push(SfxId::PlayerDied, std::string(toString(p.type)) + " defeated");
    }
    m_players.swap(livingPlayers);
}

void Game::updateCollectibles(float dt)
{
    m_collectibles.update(dt);

    int gathered = m_collectibles.collectOverlapping(m_players);
    if (gathered > 0) {
        m_economy.earn(gathered);
        m_collectibles.compact();

        char buffer[64];
        std::snprintf(buffer, sizeof(buffer), "collectible +%d (coins %d)",
                      gathered, m_economy.coins());
        push(SfxId::CollectiblePicked, buffer);
    }
}

void Game::syncUnlocks()
{
    // section 4.3: cards unlock automatically once enough points are held and
    // go back to locked if the player spends below the cost again.
    for (;;) {
        UnlockChange change = m_economy.refreshUnlocks();
        if (change == UnlockChange::None) break;

        switch (change) {
        case UnlockChange::RangedUnlocked:   push(SfxId::CardUnlocked, "Ranged unlocked");  break;
        case UnlockChange::RangedRelocked:   push(SfxId::CardRelocked, "Ranged locked");    break;
        case UnlockChange::DefenseUnlocked:  push(SfxId::CardUnlocked, "Defense unlocked"); break;
        case UnlockChange::DefenseRelocked:  push(SfxId::CardRelocked, "Defense locked");   break;
        case UnlockChange::None:             break;
        }
    }
}

void Game::checkEndConditions()
{
    // WIN: "by defeating all enemies across all three waves"
    if (m_waves.allCleared(m_enemies)) {
        push(SfxId::GameWon, "all waves cleared");
        m_sound.stopBackgroundMusic();
        m_menu.open(MenuKind::WinLose);
        setState(GameState::Won);
        return;
    }

    // LOSE: "if at least one enemy reaches the Home Base from any lane.
    //        In this case, everything in the scene stops."
    for (size_t i = 0; i < m_enemies.size(); ++i) {
        if (m_playfield.reachedHomeBase(m_enemies[i].x)) {
            char buffer[96];
            std::snprintf(buffer, sizeof(buffer),
                          "%s reached the home base in lane %d",
                          toString(m_enemies[i].type), m_enemies[i].lane);
            push(SfxId::GameLost, buffer);
            m_sound.stopBackgroundMusic();
            m_menu.open(MenuKind::WinLose);
            setState(GameState::Lost);
            return;
        }
    }
}

// --- queries --------------------------------------------------------------

Enemy* Game::blockingEnemyFor(const Player& p) const
{
    Enemy* best = 0;
    float bestDist = 0.0f;

    for (size_t i = 0; i < m_enemies.size(); ++i) {
        const Enemy& e = m_enemies[i];
        if (!e.alive() || e.lane != p.lane) continue;

        float d = std::fabs(e.x - p.x);
        if (d > ENCOUNTER_DISTANCE) continue;

        if (!best || d < bestDist) { best = const_cast<Enemy*>(&e); bestDist = d; }
    }
    return best;
}

Player* Game::blockingPlayerFor(const Enemy& e) const
{
    Player* best = 0;
    float bestDist = 0.0f;

    for (size_t i = 0; i < m_players.size(); ++i) {
        const Player& p = m_players[i];
        if (!p.alive() || p.lane != e.lane) continue;

        float d = std::fabs(p.x - e.x);
        if (d > ENCOUNTER_DISTANCE) continue;

        if (!best || d < bestDist) { best = const_cast<Player*>(&p); bestDist = d; }
    }
    return best;
}

Enemy* Game::nearestEnemyInFront(int lane, float x, float maxRange) const
{
    Enemy* best = 0;
    float bestDist = 0.0f;

    for (size_t i = 0; i < m_enemies.size(); ++i) {
        const Enemy& e = m_enemies[i];
        if (!e.alive() || e.lane != lane) continue;

        float d = e.x - x;                       // only enemies ahead (to the right)
        if (d < -ENCOUNTER_DISTANCE || d > maxRange) continue;

        if (!best || d < bestDist) { best = const_cast<Enemy*>(&e); bestDist = d; }
    }
    return best;
}

Player* Game::nearestPlayerInFront(int lane, float x, float maxRange) const
{
    Player* best = 0;
    float bestDist = 0.0f;

    for (size_t i = 0; i < m_players.size(); ++i) {
        const Player& p = m_players[i];
        if (!p.alive() || p.lane != lane) continue;

        float d = x - p.x;                       // players are to the LEFT of an enemy
        if (d < -ENCOUNTER_DISTANCE || d > maxRange) continue;

        if (!best || d < bestDist) { best = const_cast<Player*>(&p); bestDist = d; }
    }
    return best;
}

Player* Game::defenseShielding(const Player& p) const
{
    // "the Defense Player provides protection ... as long as the player
    //  remains behind the Defense Player. Once the player moves past the
    //  Defense Player ... the player will become vulnerable."
    //
    // Enemies always come from the RIGHT, so the shield has to be at an x >=
    // the player's x (it spawns on the same leftmost cell, and the moment the
    // player advances past it, the protection is gone).
    const Player* shield = findDefenseShield(m_players, p);
    return const_cast<Player*>(shield);
}

bool Game::hitPlayer(Player& p, int amount, AttackHeight height)
{
    if (!p.alive()) return false;

    // section 3.8: jumping avoids low attacks, ducking avoids high attacks.
    // (The Defense player can never jump or duck, so it never dodges.)
    if (p.dodges(height)) return false;

    // a shielded player passes the hit on to the Defense player in front
    Player* shield = defenseShielding(p);
    if (shield) {
        shield->receiveDamage(amount);
        push(SfxId::ProjectileHit, "absorbed by the Defense player");
        return true;
    }

    p.receiveDamage(amount);
    return true;
}

// --- helpers --------------------------------------------------------------

int Game::armedCard() const
{
    return m_cardArmed ? static_cast<int>(m_armedCard) : -1;
}

int Game::alivePlayerCount() const
{
    int n = 0;
    for (size_t i = 0; i < m_players.size(); ++i)
        if (m_players[i].alive()) ++n;
    return n;
}

int Game::aliveEnemyCount() const
{
    int n = 0;
    for (size_t i = 0; i < m_enemies.size(); ++i)
        if (m_enemies[i].alive()) ++n;
    return n;
}

int Game::projectilesInFlight() const
{
    return static_cast<int>(m_projectiles.size());
}

std::string Game::statusLine() const
{
    int wave = (m_waves.currentWave() < 0) ? 0 : (m_waves.currentWave() + 1);

    char buffer[512];
    std::snprintf(buffer, sizeof(buffer),
                  "%s | coins %d (earned %d) | wave %d/%d | enemies %d | players %d | shots %d | armed %d",
                  toString(m_state), m_economy.coins(), m_economy.earnedTotal(),
                  wave, m_waves.waveCount(), aliveEnemyCount(), alivePlayerCount(),
                  projectilesInFlight(), armedCard());
    return std::string(buffer);
}

void Game::push(SfxId sfx, const std::string& message)
{
    GameEvent event(sfx, message);
    m_sound.onEvent(event);
    m_events.push_back(event);
}

std::vector<GameEvent> Game::drainEvents()
{
    std::vector<GameEvent> out;
    out.swap(m_events);
    return out;
}
