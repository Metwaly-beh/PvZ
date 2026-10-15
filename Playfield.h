#ifndef PLAYFIELD_H
#define PLAYFIELD_H

// ---------------------------------------------------------------------------
// Playfield.h
//
// The grid itself (section 3.1): horizontal lanes, the home base at the left
// end, the enemy entry at the right edge, plus the INPUT state of the grid:
// which lane the mouse hovers and which lanes are selected.
//
//   * hovering a lane is what the frontend highlights,
//   * a left click toggles the selection of that lane,
//   * SEVERAL lanes can be selected at once (they are the lanes jump/duck
//     applies to, and the lanes a placed player may go to).
//
// No rendering lives here - only numbers the renderer will read.
// ---------------------------------------------------------------------------

#include <vector>

#include "Core/Types.h"

class Playfield
{
public:
    Playfield();

    // --- geometry ---------------------------------------------------------
    int   laneCount() const;
    float laneBottomY(int lane) const;   // lower edge of the lane
    float laneTopY(int lane) const;      // upper edge of the lane
    float laneCenterY(int lane) const;   // where characters sit
    int   laneAtY(float y) const;        // -1 when y is off the grid
    int   laneAtPoint(float x, float y) const; // -1 when the point is off the grid

    float leftmostX() const;             // where players are placed
    float rightmostX() const;            // where enemies emerge
    float homeBaseX() const;             // losing line
    bool  reachedHomeBase(float x) const;// x <= homeBaseX

    // --- hover & selection ------------------------------------------------
    void  setHoveredLane(int lane);
    int   hoveredLane() const;

    bool  toggleLaneSelected(int lane);  // a click on the grid; returns true if the lane is now SELECTED
    bool  isLaneSelected(int lane) const;
    void  clearSelection();
    int   selectedLaneCount() const;
    int   lastSelectedLane() const;      // most recently selected, -1 if none

private:
    int m_hoveredLane;
    std::vector<int> m_selectionOrder;   // appended on select, erased on deselect
};

#endif // PLAYFIELD_H
