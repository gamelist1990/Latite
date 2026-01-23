#pragma once
#include "../../Module.h"

#include <deque>

class Trails : public Module {
public:
    Trails();

    void onRenderLevel(RenderLevelEvent& event);

private:
    // Settings
    ValueType renderThrough = BoolValue(false);
    ValueType color = ColorValue(0.0f, 175.f/255.f, 1.f, 1.f); // #00AFFF
    ValueType opacity = FloatValue(1.f);
    ValueType sampleInterval = FloatValue(0.05f);
    ValueType maxPoints = IntValue(500);
    ValueType clearTrail = BoolValue(false);

    // Fade settings
    ValueType fade = BoolValue(true);                    // enable/disable fading
    ValueType tailOpacity = FloatValue(0.10f);           // minimum opacity for oldest point (0..1)

    // runtime
    std::deque<Vec3> m_points;
    float m_sampleTimer = 0.f;
    static constexpr float kMinPointDist = 0.02f; // meters
};
