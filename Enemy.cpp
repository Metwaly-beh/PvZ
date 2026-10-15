#include "Enemy.h"

float enemySpeed(EnemyType type)
{
    switch (type) {
    case EnemyType::EasyUpper:   return ENEMY_EASY_SPEED;
    case EnemyType::MediumLower: return ENEMY_MEDIUM_SPEED;
    case EnemyType::HardMelee:   return ENEMY_HARD_SPEED;
    }
    return ENEMY_EASY_SPEED;
}

AttackHeight enemyAttackHeight(EnemyType type)
{
    switch (type) {
    case EnemyType::EasyUpper:   return AttackHeight::High;   // dodge with DUCK
    case EnemyType::MediumLower: return AttackHeight::Low;    // dodge with JUMP
    case EnemyType::HardMelee:   return AttackHeight::Straight; // melee, cannot be dodged
    }
    return AttackHeight::Straight;
}

int enemyMaxHp(EnemyType type)
{
    switch (type) {
    case EnemyType::EasyUpper:   return EASY_HP;
    case EnemyType::MediumLower: return MEDIUM_HP;
    case EnemyType::HardMelee:   return HARD_HP;
    }
    return EASY_HP;
}

int enemyDamage(EnemyType type)
{
    // "All enemies deal 1 point of damage per attack, except for the
    //  Hard - Melee Enemy, which deals 2 points of damage per attack."
    return (type == EnemyType::HardMelee) ? HARD_ENEMY_DAMAGE : ENEMY_DAMAGE;
}

int enemyCoinDrop(EnemyType type)
{
    switch (type) {
    case EnemyType::EasyUpper:   return COINS_EASY_DROP;
    case EnemyType::MediumLower: return COINS_MEDIUM_DROP;
    case EnemyType::HardMelee:   return COINS_HARD_DROP;
    }
    return COINS_EASY_DROP;
}

Enemy makeEnemy(int id, EnemyType type, int lane, float x)
{
    Enemy e;
    e.id = id;
    e.type = type;
    e.lane = lane;
    e.x = x;
    e.maxHp = enemyMaxHp(type);
    e.hp = e.maxHp;
    e.speed = enemySpeed(type);
    e.attackHeight = enemyAttackHeight(type);
    e.state = CharState::Advancing;   // enemies start walking immediately
    return e;
}
