#include "CharacterSprites.h"
#include <cmath>
#include <algorithm>
#include "Canvas.h"
#include "Game.h"
#include "SpriteAtlas.h"

namespace {

CharArt makeArt(const char* name,
                SpriteAnim idle, SpriteAnim walk,
                SpriteAnim attack, SpriteAnim death)
{
    const SpriteSheet* s = findSpriteSheet(name);

    // Handle missing sheets without being too systematic
    if (!s) {
        CharArt fallback;
        fallback.sheet.name = name;
        fallback.sheet.fw = 32; fallback.sheet.fh = 32;
        fallback.sheet.cols = 1; fallback.sheet.rows = 1;
        fallback.sheet.ox = 0; fallback.sheet.oy = 0;
        fallback.flipX = false;
        fallback.idle = idle; fallback.walk = walk;
        fallback.attack = attack; fallback.death = death;
        return fallback;
    }

    CharArt art;
    art.sheet.name = name;
    art.sheet.fw = s->fw;
    art.sheet.fh = s->fh;
    art.sheet.cols = s->cols;
    art.sheet.rows = s->rows;
    art.sheet.ox = s->x;
    art.sheet.oy = s->y;
    art.flipX = (s->flipX != 0);
    art.idle = idle;
    art.walk = walk;
    art.attack = attack;
    art.death = death;
    return art;
}

const SpriteAnim& animationFor(const CharArt& art, CharState state)
{
    switch (state) {
        case CharState::Dead: return art.death;
        case CharState::Attacking:
        case CharState::Fighting: return art.attack;
        case CharState::Advancing:
        case CharState::Jumping: return art.walk;
        case CharState::Ducking:
        case CharState::Idle: return art.idle;
        default: return art.idle;
    }
}

void drawOne(Canvas& solid, const CharArt& art, float x, float y,
             float animTime, CharState state, float hitFlash, float bob,
             float size)
{
    const SpriteAnim& anim = animationFor(art, state);
    int frame = getCurrentFrame(anim, animTime, 0.0f);

    int cells = art.sheet.cols * art.sheet.rows;
    if (frame < 0) frame = 0;
    if (cells > 0 && frame >= cells) frame = cells - 1;

    int col = art.sheet.cols > 0 ? frame % art.sheet.cols : 0;
    int row = art.sheet.cols > 0 ? frame / art.sheet.cols : 0;

    float px = (float)(art.sheet.ox + col * art.sheet.fw);
    float py = (float)(art.sheet.oy + row * art.sheet.fh);

    float u0 = px / kSpriteAtlasWidth;
    float u1 = (px + art.sheet.fw) / kSpriteAtlasWidth;
    float v0 = 1.0f - (py + art.sheet.fh) / kSpriteAtlasHeight;
    float v1 = 1.0f - py / kSpriteAtlasHeight;

    float flash = std::min(hitFlash * 3.0f, 1.0f);
    Color tint(1.0f, 1.0f - 0.72f * flash, 1.0f - 0.72f * flash, 1.0f);

    solid.drawSprite(u0, v0, u1, v1,
                     x - size * 0.5f, y - size * 0.5f + bob,
                     size, size, tint, art.flipX);
}

} // namespace

const CharArt p_melee = makeArt("p_melee", SpriteAnim(0, 13, 0.22f, true), SpriteAnim(13, 8, 0.12f, true), SpriteAnim(21, 10, 0.09f, true), SpriteAnim(31, 11, 0.14f, false));
const CharArt p_ranged = makeArt("p_ranged", SpriteAnim(0, 6, 0.24f, true), SpriteAnim(6, 8, 0.13f, true), SpriteAnim(14, 9, 0.10f, true), SpriteAnim(23, 7, 0.14f, false));
const CharArt p_tank = makeArt("p_tank", SpriteAnim(0, 6, 0.26f, true), SpriteAnim(6, 8, 0.14f, true), SpriteAnim(14, 5, 0.14f, true), SpriteAnim(19, 8, 0.16f, false));
const CharArt e_tank = makeArt("e_tank", SpriteAnim(0, 4, 0.26f, true), SpriteAnim(4, 5, 0.16f, true), SpriteAnim(9, 6, 0.11f, true), SpriteAnim(15, 6, 0.16f, false));
const CharArt e_ranged = makeArt("e_ranged", SpriteAnim(0, 4, 0.26f, true), SpriteAnim(4, 6, 0.15f, true), SpriteAnim(10, 6, 0.10f, true), SpriteAnim(16, 7, 0.14f, false));
const CharArt e_swarm = makeArt("e_swarm", SpriteAnim(0, 4, 0.10f, true), SpriteAnim(4, 4, 0.08f, true), SpriteAnim(8, 6, 0.08f, true), SpriteAnim(14, 1, 0.20f, false));

const CharArt& artFor(PlayerType type) {
    switch (type) {
        case PlayerType::Melee:   return p_melee;
        case PlayerType::Ranged:  return p_ranged;
        case PlayerType::Defense: return p_tank;
        default: return p_melee;
    }
}

const CharArt& artFor(EnemyType type) {
    switch (type) {
        case EnemyType::EasyUpper:   return e_ranged;
        case EnemyType::MediumLower: return e_swarm;
        case EnemyType::HardMelee:   return e_tank;
        default: return e_tank;
    }
}

void drawCharacters(const Game& game, Canvas& solid, float /*time*/) {
    const float SIZE = 1.2f;
    for (const auto& p : game.players()) {
        float y = game.playfield().laneCenterY(p.lane);
        float bob = (p.state == CharState::Advancing) ? std::sin(p.animTime * 8.0f) * 0.045f : 0.0f;
        drawOne(solid, artFor(p.type), p.x, y, p.animTime, p.state, p.hitFlash, bob, SIZE);
    }
    for (const auto& e : game.enemies()) {
        float y = game.playfield().laneCenterY(e.lane);
        float bob = (e.state == CharState::Advancing) ? std::sin(e.animTime * 7.0f) * 0.035f : 0.0f;
        drawOne(solid, artFor(e.type), e.x, y, e.animTime, e.state, e.hitFlash, bob, SIZE);
    }
}
