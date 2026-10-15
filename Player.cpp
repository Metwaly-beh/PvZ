#include "Player.h"

Player makePlayer(int id, PlayerType type, int lane, float x)
{
    Player p;
    p.id = id;
    p.type = type;
    p.lane = lane;
    p.x = x;

    switch (type) {
    case PlayerType::Melee:
        p.maxHp = MELEE_HP;
        p.state = CharState::Advancing;
        break;
    case PlayerType::Ranged:
        p.maxHp = RANGED_HP;
        p.state = CharState::Advancing;
        break;
    case PlayerType::Defense:
        p.maxHp = DEFENSE_HP;
        p.state = CharState::Idle;   // it never moves after being placed
        break;
    }

    p.hp = p.maxHp;
    return p;
}

int commandJump(std::vector<Player>& players, const Playfield& playfield)
{
    int told = 0;
    for (size_t i = 0; i < players.size(); ++i) {
        Player& p = players[i];
        if (!p.alive()) continue;
        if (!p.canChangeStance()) continue;              // defense never jumps
        if (!playfield.isLaneSelected(p.lane)) continue; // only SELECTED lanes
        p.jumpTimer = JUMP_TIME;
        p.duckTimer = 0.0f;                              // a jump cancels a duck
        p.state = CharState::Jumping;
        ++told;
    }
    return told;
}

int commandDuck(std::vector<Player>& players, const Playfield& playfield)
{
    int told = 0;
    for (size_t i = 0; i < players.size(); ++i) {
        Player& p = players[i];
        if (!p.alive()) continue;
        if (!p.canChangeStance()) continue;
        if (!playfield.isLaneSelected(p.lane)) continue;
        p.duckTimer = DUCK_TIME;
        p.jumpTimer = 0.0f;                              // a duck cancels a jump
        p.state = CharState::Ducking;
        ++told;
    }
    return told;
}

void advancePlayer(Player& p, float dt, float destX)
{
    if (!p.alive() || !p.canMove()) return;

    // stance (jumping / ducking) never blocks the advance, it only changes
    // which state the frontend should animate.
    p.x += PLAYER_ADVANCE_SPEED * dt;

    // "players advance ... until they reach the right"
    if (p.x > destX) p.x = destX;

    p.state = (p.jumpTimer > 0.0f) ? CharState::Jumping
            : (p.duckTimer > 0.0f) ? CharState::Ducking
                                   : CharState::Advancing;
}

const Player* findDefenseShield(const std::vector<Player>& players, const Player& p)
{
    // a Defense player is never protected by another Defense player
    if (p.type == PlayerType::Defense) return 0;
    if (!p.alive()) return 0;

    for (size_t i = 0; i < players.size(); ++i) {
        const Player& d = players[i];
        if (!d.alive()) continue;
        if (d.type != PlayerType::Defense) continue;
        if (d.lane != p.lane) continue;
        if (d.x >= p.x) return &d;      // it stands between p and the enemies
    }
    return 0;
}
