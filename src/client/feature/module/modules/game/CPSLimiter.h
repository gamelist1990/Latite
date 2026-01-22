#pragma once
#include <chrono>
#include <vector>
#include <memory>
#include "../../Module.h"
#include "client/feature/setting/Setting.h"

// Limits clicks-per-second for mouse buttons
class CPSLimiter : public Module {
public:
    CPSLimiter();
    ~CPSLimiter() = default;

private:
    void onClick(Event& ev);

    // settings
    ValueType maxCPS = FloatValue(12.f);
    EnumData targetButton; // 0 = Left, 1 = Right, 2 = Both

    // runtime
    // RateLimiter is intentionally reference-counted and internally mutex-protected
    // (semantics similar to Arc<Mutex<RateLimiter>>). The implementation lives in the .cpp.
    struct RateLimiter;
    std::shared_ptr<RateLimiter> rateLimiter;
};
