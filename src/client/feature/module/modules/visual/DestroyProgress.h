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
};