#include "Economy.h"

Economy::Economy()
    : m_coins(0), m_earnedTotal(0),
      m_rangedUnlocked(false), m_defenseUnlocked(false)
{
    reset();
}

void Economy::reset()
{
    m_coins = STARTING_COINS;
    m_earnedTotal = 0;             // nothing has been COLLECTED yet
    m_rangedUnlocked = false;      // "Initially, all player types are locked
    m_defenseUnlocked = false;     //  except for the Melee Player"
}

int Economy::costOf(PlayerType type)
{
    switch (type) {
    case PlayerType::Melee:   return MELEE_COST;
    case PlayerType::Ranged:  return RANGED_COST;
    case PlayerType::Defense: return DEFENSE_COST;
    }
    return MELEE_COST;
}

bool Economy::isUnlocked(PlayerType type) const
{
    if (type == PlayerType::Melee) return true;    // the starter card

    // A card unlocks once the player has COLLECTED enough points AND still
    // HOLDS at least that many points (re-lock rule of section 4.3).
    int cost = costOf(type);
    bool held = (m_coins >= cost);
    bool gathered = (m_earnedTotal >= cost);

    if (type == PlayerType::Ranged)  return held && gathered;
    if (type == PlayerType::Defense) return held && gathered;
    return false;
}

bool Economy::canAfford(PlayerType type) const
{
    return m_coins >= costOf(type);
}

bool Economy::spend(PlayerType type)
{
    if (!isUnlocked(type)) return false;
    int cost = costOf(type);
    if (m_coins < cost) return false;
    m_coins -= cost;
    return true;
}

void Economy::earn(int amount)
{
    if (amount <= 0) return;
    m_coins += amount;
    m_earnedTotal += amount;
}

UnlockChange Economy::refreshUnlocks()
{
    bool rangedNow = isUnlocked(PlayerType::Ranged);
    if (rangedNow != m_rangedUnlocked) {
        m_rangedUnlocked = rangedNow;
        return rangedNow ? UnlockChange::RangedUnlocked
                         : UnlockChange::RangedRelocked;
    }

    bool defenseNow = isUnlocked(PlayerType::Defense);
    if (defenseNow != m_defenseUnlocked) {
        m_defenseUnlocked = defenseNow;
        return defenseNow ? UnlockChange::DefenseUnlocked
                          : UnlockChange::DefenseRelocked;
    }

    return UnlockChange::None;
}
