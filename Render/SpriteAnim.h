#ifndef RENDER_SPRITEANIM_H
#define RENDER_SPRITEANIM_H

// ---------------------------------------------------------------------------
// SpriteAnim.h
//
// Simple animation data for a university-level project.
// ---------------------------------------------------------------------------

struct SpriteAnim {
    int startFrame;     // First frame index in the sheet
    int frameCount;     // Number of frames in animation
    float frameDuration; // Seconds per frame
    bool looping;       // True for Idle/Walk, false for Attack/Death

    SpriteAnim() : startFrame(0), frameCount(1), frameDuration(0.1f), looping(true) {}
    SpriteAnim(int s, int c, float d, bool l) 
        : startFrame(s), frameCount(c), frameDuration(d), looping(l) {}
};

// Helper to calculate the current frame based on elapsed time and animation state.
inline int getCurrentFrame(const SpriteAnim& anim, float time, float startTime) {
    float elapsed = time - startTime;
    if (elapsed < 0) return anim.startFrame;
    
    int frameOffset = static_cast<int>(elapsed / anim.frameDuration);
    if (anim.looping) {
        return anim.startFrame + (frameOffset % anim.frameCount);
    } else {
        return anim.startFrame + std::min(frameOffset, anim.frameCount - 1);
    }
}

#endif // RENDER_SPRITEANIM_H
