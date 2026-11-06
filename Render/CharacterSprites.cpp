#include "CharacterSprites.h"

#include <cmath>

#include "Canvas.h"
#include "Game.h"
#include "SpriteAtlas.h"

// ---------------------------------------------------------------------------
// Frame ranges below were emitted by Tools/gen_sprite_atlas.py:
//
//   p_melee   42 frames  idle 0..12   walk 13..20  attack 21..30  death 31..41
//   p_ranged  30 frames  idle 0..5    walk 6..13   attack 14..22  death 23..29
//   p_tank    27 frames  idle 0..5    walk 6..13   attack 14..18  death 19..26
//   e_tank    21 frames  idle 0..3    walk 4..8    attack 9..14   death 15..20
//   e_ranged  23 frames  idle 0..3    walk 4..9    attack 10..15  death 16..22
//   e_swarm   15 frames  idle 0..3    walk 4..7    attack 8..13   death 14..14
// ---------------------------------------------------------------------------

namespace {

CharArt makeArt(const char* name,
                SpriteAnim idle, SpriteAnim walk,
                SpriteAnim attack, SpriteAnim death)
{
    const SpriteSheet* s = findSpriteSheet(name);

    CharArt art;
    art.sheet.name = name;
    art.sheet.fw   = s ? s->fw : 32;
    art.sheet.fh   = s ? s->fh : 32;
    art.sheet.cols = s ? s->cols : 1;
    art.sheet.rows = s ? s->rows : 1;
    art.sheet.ox   = s ? s->x : 0;
    art.sheet.oy   = s ? s->y : 0;
    art.flipX      = (s != 0 && s->flipX != 0);
    art.idle   = idle;
    art.walk   = walk;
    art.attack = attack;
    art.death  = death;
    return art;
}

// A character that is stopped is swinging, one that is moving is walking and
// one that met the other side is trading blows - the simulation already
// tells us which, so the frontend only has to map states onto ranges.
const SpriteAnim& animationFor(const CharArt& art, CharState state)
{
    switch (state) {
    case CharState::Dead:      return art.death;
    case CharState::Attacking: return art.attack;
    case CharState::Fighting:  return art.attack;
    case CharState::Advancing: return art.walk;
    case CharState::Jumping:   return art.walk;
    case CharState::Ducking:   return art.idle;
    case CharState::Idle:      return art.idle;
    }
    return art.idle;
}

void drawOne(Canvas& solid, const CharArt& art, float x, float y,
             float animTime, CharState state, float hitFlash, float bob,
             float size)
{
    const SpriteAnim& anim = animationFor(art, state);

    int frame = getCurrentFrame(anim, animTime, 0.0f);
    const int cells = art.sheet.cols * art.sheet.rows;
    if (cells > 0) {
        if (frame < 0) frame = 0;
        if (frame >= cells) frame = cells - 1;
    }
    else {
        frame = 0;
    }

    const int col = art.sheet.cols > 0 ? frame % art.sheet.cols : 0;
    const int row = art.sheet.cols > 0 ? frame / art.sheet.cols : 0;
    const float px = static_cast<float>(art.sheet.ox + col * art.sheet.fw);
    const float py = static_cast<float>(art.sheet.oy + row * art.sheet.fh);
    const float atlasW = static_cast<float>(kSpriteAtlasWidth);
    const float atlasH = static_cast<float>(kSpriteAtlasHeight);

    // The atlas is uploaded flipped, so v grows upwards: the top of the
    // frame gets the larger v and lands on the top of the quad.
    const float u0 = px / atlasW;
    const float u1 = (px + art.sheet.fw) / atlasW;
    const float v0 = 1.0f - (py + art.sheet.fh) / atlasH;
    const float v1 = 1.0f - py / atlasH;

    // Damage feedback: a red wash over the art that fades with hitFlash.
    float flash = hitFlash * 3.0f;
    if (flash > 1.0f) flash = 1.0f;
    const Color tint(1.0f, 1.0f - 0.72f * flash, 1.0f - 0.72f * flash, 1.0f);

    solid.drawSprite(u0, v0, u1, v1,
                     x - size * 0.5f, y - size * 0.5f + bob,
                     size, size, tint, art.flipX);
}

} // namespace

const CharArt p_melee = makeArt("p_melee",
    SpriteAnim(0, 13, 0.22f, true),
    SpriteAnim(13, 8, 0.12f, true),
    SpriteAnim(21, 10, 0.09f, true),
    SpriteAnim(31, 11, 0.14f, false));

const CharArt p_ranged = makeArt("p_ranged",
    SpriteAnim(0, 6, 0.24f, true),
    SpriteAnim(6, 8, 0.13f, true),
    SpriteAnim(14, 9, 0.10f, true),
    SpriteAnim(23, 7, 0.14f, false));

const CharArt p_tank = makeArt("p_tank",
    SpriteAnim(0, 6, 0.26f, true),
    SpriteAnim(6, 8, 0.14f, true),
    SpriteAnim(14, 5, 0.14f, true),
    SpriteAnim(19, 8, 0.16f, false));

const CharArt e_tank = makeArt("e_tank",
    SpriteAnim(0, 4, 0.26f, true),
    SpriteAnim(4, 5, 0.16f, true),
    SpriteAnim(9, 6, 0.11f, true),
    SpriteAnim(15, 6, 0.16f, false));

const CharArt e_ranged = makeArt("e_ranged",
    SpriteAnim(0, 4, 0.26f, true),
    SpriteAnim(4, 6, 0.15f, true),
    SpriteAnim(10, 6, 0.10f, true),
    SpriteAnim(16, 7, 0.14f, false));

const CharArt e_swarm = makeArt("e_swarm",
    SpriteAnim(0, 4, 0.10f, true),
    SpriteAnim(4, 4, 0.08f, true),
    SpriteAnim(8, 6, 0.08f, true),
    SpriteAnim(14, 1, 0.20f, false));

const CharArt& artFor(PlayerType type)
{
    switch (type) {
    case PlayerType::Melee:   return p_melee;
    case PlayerType::Ranged:  return p_ranged;
    case PlayerType::Defense: return p_tank;
    }
    return p_melee;
}

const CharArt& artFor(EnemyType type)
{
    switch (type) {
    case EnemyType::EasyUpper:   return e_ranged;
    case EnemyType::MediumLower: return e_swarm;
    case EnemyType::HardMelee:   return e_tank;
    }
    return e_tank;
}

void drawCharacters(const Game& game, Canvas& solid, float /*time*/)
{
    // Lanes are LANE_HEIGHT (1.5) tall, so a little over one unit keeps a
    // character inside its lane while still reading clearly on screen.
    const float SIZE = 1.2f;

    for (size_t i = 0; i < game.players().size(); ++i) {
        const Player& p = game.players()[i];
        const float y = game.playfield().laneCenterY(p.lane);
        const float bob = p.state == CharState::Advancing
            ? std::sin(p.animTime * 8.0f) * 0.045f : 0.0f;
        drawOne(solid, artFor(p.type), p.x, y, p.animTime, p.state,
                p.hitFlash, bob, SIZE);
    }

    for (size_t i = 0; i < game.enemies().size(); ++i) {
        const Enemy& e = game.enemies()[i];
        const float y = game.playfield().laneCenterY(e.lane);
        const float bob = e.state == CharState::Advancing
            ? std::sin(e.animTime * 7.0f) * 0.035f : 0.0f;
        drawOne(solid, artFor(e.type), e.x, y, e.animTime, e.state,
                e.hitFlash, bob, SIZE);
    }
}
