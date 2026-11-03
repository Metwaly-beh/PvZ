#ifndef RENDER_SPRITEANIM_H
#define RENDER_SPRITEANIM_H

struct SpriteAnim {
    int start;
    int count;
    float duration;
    bool loop;

    SpriteAnim() : start(0), count(1), duration(0.1f), loop(true) {}
    SpriteAnim(int s, int c, float d, bool l) : start(s), count(c), duration(d), loop(l) {}
};

inline int getCurrentFrame(const SpriteAnim& anim, float time, float startT) {
    float elapsed = time - startT;
    if (elapsed < 0) return anim.start;
    
    int frame = (int)(elapsed / anim.duration);
    if (anim.loop) {
        return anim.start + (frame % anim.count);
    }
    return anim.start + (frame < anim.count ? frame : anim.count - 1);
}
#endif
