#ifndef ECONOMY_H
#define ECONOMY_H

// ---------------------------------------------------------------------------
// Economy.h  (sections 3.2 and 4.3 - coins, player cards, unlocking)
//
// Rules taken from the description:
//
//   * the game STARTS with enough coins for at least three Melee players,
//     so MELEE is the only card that is unlocked from the beginning;
//   * "As the player collects enough coins, the other player types are
//     progressively unlocked"        -> unlock when TOTAL COLLECTED >= cost;
//   * "If the player's available points/collectibles later fall below the
//     required unlock cost, the corresponding player card must automatically
//     return to its locked state"    -> re-lock when CURRENT COINS < cost;
//   * costs: Ranged = 2x Melee, Defense = 3x Melee (page 9 notes column).
//
// A locked card must not respond to hover / selection at all, so the very
// first thing selectCard() checks is isUnlocked().
// ---------------------------------------------------------------------------

#include <string>
#include <vector>

#include "Core/Types.h"

// one transition of one card, reported by Economy::refreshUnlocks()
enum class UnlockChange
{
    None,
    RangedUnlocked,
    RangedRelocked,
    DefenseUnlocked,
    DefenseRelocked
};

class Economy
{
public:
    Economy();

    void reset();                       // start a fresh game (30 coins, melee only)

    int  coins() const { return m_coins; }
    int  earnedTotal() const { return m_earnedTotal; }

    static int  costOf(PlayerType type);

    bool isUnlocked(PlayerType type) const;
    bool canAfford(PlayerType type) const;

    bool spend(PlayerType type);        // false when locked / too expensive
    void earn(int amount);              // from picked up collectibles

    // re-evaluates every card; reports ONE transition per call (call it in a
    // loop until it returns None) so the game can emit CardUnlocked /
    // CardRelocked events
    UnlockChange refreshUnlocks();

private:
    int m_coins;
    int m_earnedTotal;

    // last state we reported, used to detect unlock / re-lock transitions
    bool m_rangedUnlocked;
    bool m_defenseUnlocked;
};

#endif // ECONOMY_H
