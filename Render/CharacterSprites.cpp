#include "CharacterSprites.h"
#include <cmath>
#include <algorithm>
#include "Canvas.h"
#include "Cutscene.h"
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

void drawCutsceneCharacters(const Cutscene& cutscene, Canvas& solid, Canvas& light)
{
    const std::vector<CutsceneActor>& actors = cutscene.actors();
    const float size = 1.8f;

    for (size_t i = 0; i < actors.size(); ++i) {
        const CutsceneActor& actor = actors[i];
        if (!actor.visible) continue;

        const CharArt& art = actor.isPlayer
            ? artFor(PlayerType::Melee) : artFor(EnemyType::HardMelee);
        const CharState state = actor.anim == "walk" ? CharState::Advancing
            : actor.anim == "attack" ? CharState::Attacking
            : actor.anim == "dead" ? CharState::Dead : CharState::Idle;
        const float bob = state == CharState::Advancing
            ? std::sin(cutscene.time() * 8.0f + static_cast<float>(i)) * 0.07f
            : 0.0f;

        solid.ellipse(actor.pos.x, actor.pos.y - size * 0.43f,
                      size * 0.35f, 0.12f, 0.0f,
                      Color(0.02f, 0.035f, 0.025f, 0.55f), 24);
        drawOne(solid, art, actor.pos.x, actor.pos.y, cutscene.time(),
                state, 0.0f, bob, size);

        if (actor.name == cutscene.currentSpeaker()) {
            const float pulse = 0.5f + 0.5f * std::sin(cutscene.time() * 10.0f);
            const float bubbleY = actor.pos.y + 1.28f + pulse * 0.05f;
            const Color bubble(0.97f, 0.91f, 0.71f, 0.96f);
            solid.roundedRect(actor.pos.x - 0.62f, bubbleY - 0.16f,
                              1.24f, 0.42f, 0.12f, bubble);
            solid.poly({Vec2(actor.pos.x - 0.10f, bubbleY - 0.15f),
                        Vec2(actor.pos.x + 0.12f, bubbleY - 0.15f),
                        Vec2(actor.pos.x + 0.02f, bubbleY - 0.34f)}, bubble);
            for (int dot = 0; dot < 3; ++dot)
                solid.circle(actor.pos.x - 0.24f + dot * 0.24f, bubbleY + 0.05f,
                             0.045f + pulse * 0.008f, Color(0.20f, 0.32f, 0.22f), 12);
        }
    }

    if (cutscene.time() >= 9.0f && cutscene.time() <= 10.4f && actors.size() >= 2) {
        const float clashX = (actors[0].pos.x + actors[1].pos.x) * 0.5f;
        const float pulse = 0.5f + 0.5f * std::sin(cutscene.time() * 24.0f);
        light.glow(clashX, actors[0].pos.y + 0.2f, 0.42f + pulse * 0.18f,
                   Color(1.0f, 0.62f, 0.18f, 0.78f));
        solid.line(Vec2(clashX - 0.28f, actors[0].pos.y - 0.15f),
                   Vec2(clashX + 0.25f, actors[0].pos.y + 0.4f),
                   0.07f, Color(1.0f, 0.88f, 0.50f, 0.95f));
        solid.line(Vec2(clashX + 0.28f, actors[0].pos.y - 0.15f),
                   Vec2(clashX - 0.25f, actors[0].pos.y + 0.4f),
                   0.07f, Color(1.0f, 0.88f, 0.50f, 0.95f));
    }
}
