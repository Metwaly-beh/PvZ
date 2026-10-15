#include "Projectile.h"

Projectile makeProjectile(int id, Faction faction, int lane, float x,
                          float laneCenterY, float vx, AttackHeight height,
                          int damage, int sourceId)
{
    Projectile p;
    p.id = id;
    p.faction = faction;
    p.lane = lane;
    p.x = x;
    p.y = laneCenterY;
    p.vx = vx;
    p.originX = x;                 // the weapon's reach is measured from here
    p.range = RANGED_RANGE;
    p.height = height;
    p.damage = damage;
    p.sourceId = sourceId;
    p.active = true;
    return p;
}
