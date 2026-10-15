#ifndef ENEMY_H
#define ENEMY_H

// ---------------------------------------------------------------------------
// Enemy.h  (section 3.3 - Enemy Logic)
//
//   EasyUpper   : normal speed, ranged shots from ABOVE (dodge with DUCK), 5 lives
//   MediumLower : normal speed, ranged shots from BELOW (dodge with JUMP), 5 lives
//   HardMelee   : runs at the player, close range swings, 7 lives, 2 damage
//
// All enemies deal 1 damage per attack except the hard melee enemy (2).
// An enemy spawns on the right edge in a RANDOM lane and must stay on that
// lane for its whole life (no lane switching, section 3.1).
// ---------------------------------------------------------------------------

#include "Character.h"
#include "Core/Types.h"

struct Enemy : public CharacterBase
{
    EnemyType    type;
    float        speed;         // units per second, moving towards -x
    AttackHeight attackHeight;  // what kind of shot this enemy fires

    Enemy()
        : type(EnemyType::EasyUpper), speed(ENEMY_EASY_SPEED),
          attackHeight(AttackHeight::High)
    {
    }

    // enemies never jump or duck - the spec only gives that tool to players
    bool dodges(AttackHeight h) const
    {
        (void)h;
        return false;
    }

    bool isRanged() const { return type != EnemyType::HardMelee; }

    void tickEnemy(float dt) { tick(dt); }
};

Enemy     makeEnemy(int id, EnemyType type, int lane, float x);
float     enemySpeed(EnemyType type);
AttackHeight enemyAttackHeight(EnemyType type);
int       enemyMaxHp(EnemyType type);
int       enemyDamage(EnemyType type);       // 1, 1, 2
int       enemyCoinDrop(EnemyType type);     // value of the collectible it drops

#endif // ENEMY_H
