#pragma once
#include "../../Module.h"

class DestroyProgress : public Module {
public:
    DestroyProgress();

    void onRenderLevel(RenderLevelEvent& event);

private:
    ValueType renderThrough = BoolValue(false);
    ValueType transparent = BoolValue(false);
    ValueType color = ColorValue(1.f, 0.8f, 0.f, 1.f);
    ValueType filled = BoolValue(true);
    ValueType opacity = FloatValue(0.85f);
    ValueType speed = FloatValue(12.f);

    // animation state
    BlockPos m_lastPos{ INT_MIN, INT_MIN, INT_MIN };
    float m_animProgress = 0.f;
    float m_popTimer = 0.f; // short pop animation after break complete

    // resilience: keep last valid blockpos for a few frames when HitResult is transiently invalid
    int m_lastPosHold = 0; // frames remaining to keep m_lastPos
    static constexpr int kLastPosHoldFrames = 6;

    // log throttling to avoid spamming latest.log when HitResult is noisy
    int m_logSuppress = 0; // frames to suppress repeated warnings
    static constexpr int kLogSuppressFrames = 30;
};