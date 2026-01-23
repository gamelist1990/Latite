#pragma once

#include <client/feature/module/Module.h>
#include <chrono>

class Hitping : public Module {
public:
    Hitping();
    virtual ~Hitping() = default;

private:
    // Settings
    ValueType soundId = TextValue(L"random.pop");
    ValueType volume = FloatValue(1.f);
    ValueType pitch = FloatValue(1.f);
    EnumData targetMode; // 0 = All, 1 = Players only
    ValueType cooldownEnabled = BoolValue(false);
    ValueType cooldownMs = FloatValue(150.f);

    // runtime state
    uint64_t lastRuntimeId = 0;
    bool lastWasPlayer = false;
    bool hasHit = false;
    std::chrono::steady_clock::time_point lastPingTime = std::chrono::steady_clock::time_point();

    void onAttack(Event& ev);
    void onPacketReceive(Event& ev);
};
