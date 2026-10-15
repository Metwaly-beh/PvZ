#include "Playfield.h"

Playfield::Playfield()
    : m_hoveredLane(-1)
{
}

int Playfield::laneCount() const
{
    return LANE_COUNT;
}

float Playfield::laneBottomY(int lane) const
{
    if (lane < 0 || lane >= LANE_COUNT) return 0.0f;
    return static_cast<float>(lane) * LANE_HEIGHT;
}

float Playfield::laneTopY(int lane) const
{
    return laneBottomY(lane) + LANE_HEIGHT;
}

float Playfield::laneCenterY(int lane) const
{
    return laneBottomY(lane) + 0.5f * LANE_HEIGHT;
}

int Playfield::laneAtY(float y) const
{
    // lanes stack from the bottom (lane 0) to the top (lane LANE_COUNT-1)
    if (y < 0.0f || y >= PLAYFIELD_HEIGHT) return -1;      // cursor is off the grid
    int lane = static_cast<int>(y / LANE_HEIGHT);
    if (lane < 0 || lane >= LANE_COUNT) return -1;
    return lane;
}

int Playfield::laneAtPoint(float x, float y) const
{
    // a lane is only highlighted while the cursor is over the GRID itself,
    // so hovering the coin counter or a card row never lights a lane up
    if (x < 0.0f || x > PLAYFIELD_WIDTH) return -1;
    return laneAtY(y);
}

float Playfield::leftmostX() const
{
    return PLAYER_SPAWN_X;
}

float Playfield::rightmostX() const
{
    return ENEMY_SPAWN_X;
}

float Playfield::homeBaseX() const
{
    return HOME_BASE_X;
}

bool Playfield::reachedHomeBase(float x) const
{
    // section 3.4: the game is lost as soon as ONE enemy reaches the base
    return x <= HOME_BASE_X;
}

void Playfield::setHoveredLane(int lane)
{
    m_hoveredLane = (lane >= 0 && lane < LANE_COUNT) ? lane : -1;
}

int Playfield::hoveredLane() const
{
    return m_hoveredLane;
}

bool Playfield::toggleLaneSelected(int lane)
{
    if (lane < 0 || lane >= LANE_COUNT) return false;

    for (size_t i = 0; i < m_selectionOrder.size(); ++i) {
        if (m_selectionOrder[i] == lane) {
            m_selectionOrder.erase(m_selectionOrder.begin() + i);
            return false;                       // it is deselected now
        }
    }

    m_selectionOrder.push_back(lane);           // it is selected now
    return true;
}

bool Playfield::isLaneSelected(int lane) const
{
    for (size_t i = 0; i < m_selectionOrder.size(); ++i)
        if (m_selectionOrder[i] == lane) return true;
    return false;
}

void Playfield::clearSelection()
{
    m_selectionOrder.clear();
}

int Playfield::selectedLaneCount() const
{
    return static_cast<int>(m_selectionOrder.size());
}

int Playfield::lastSelectedLane() const
{
    if (m_selectionOrder.empty()) return -1;
    return m_selectionOrder.back();
}
