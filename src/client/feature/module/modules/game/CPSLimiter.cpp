#include "pch.h"
#include "CPSLimiter.h"
#include "client/Latite.h"

CPSLimiter::CPSLimiter() : Module("CPSLimiter", LocalizeString::get("client.module.cpsLimiter.name"),
                                 LocalizeString::get("client.module.cpsLimiter.desc"), GAME, nokeybind) {
    targetButton.addEntry(EnumEntry(0, L"Left", L"Left mouse button"));
    targetButton.addEntry(EnumEntry(1, L"Right", L"Right mouse button"));
    targetButton.addEntry(EnumEntry(2, L"Both", L"Both mouse buttons"));

    addSliderSetting("maxCPS", L"Max CPS", L"Maximum allowed clicks per second", maxCPS, FloatValue(1.f), FloatValue(50.f), FloatValue(1.f));
    addEnumSetting("target", L"Target", L"Which mouse button to limit", targetButton);

    listen<ClickEvent>(static_cast<EventListenerFunc>(&CPSLimiter::onClick));
}

// Token-bucket rate limiter implementation (fractional tokens, thread-safe).
// Behaviour notes:
// - rate <= 0 -> unlimited (always allow)
// - capacity (burst) == rate (initial tokens = rate)
// - fractional tokens preserved between calls
// - protected by internal mutex (shared_ptr acts like Arc)
struct CPSLimiter::RateLimiter {
    std::mutex m;
    double tokens;            // fractional tokens available
    double rate;              // tokens added per second (CPS)
    std::chrono::steady_clock::time_point last;

    explicit RateLimiter(double r = 12.0)
        : tokens(r > 0.0 ? r : 0.0), rate(r), last(std::chrono::steady_clock::now()) {}

    // Update rate (keeps tokens clamped to new capacity)
    void setRate(double r) {
        std::lock_guard<std::mutex> lk(m);
        rate = r;
        if (tokens > rate) tokens = rate;
        if (rate <= 0.0) tokens = 0.0; // unlimited handled by check in tryConsume
    }

    // Try to consume one token. Returns true if allowed.
    bool tryConsumeOne(std::chrono::steady_clock::time_point now) {
        std::lock_guard<std::mutex> lk(m);
        if (rate <= 0.0) return true; // unlimited

        using seconds_d = std::chrono::duration<double>;
        double elapsed = seconds_d(now - last).count();
        last = now;

        // refill (capacity == rate)
        tokens += elapsed * rate;
        if (tokens > rate) tokens = rate;

        if (tokens >= 1.0) {
            tokens -= 1.0;
            return true;
        }
        return false;
    }
};

void CPSLimiter::onClick(Event& evGeneric) {
    auto& ev = reinterpret_cast<ClickEvent&>(evGeneric);

    // only care about press events
    if (!ev.isDown()) return;

    const int btn = ev.getMouseButton();
    const int sel = targetButton.getSelectedKey();

    const bool checkLeft = (sel == 0 || sel == 2) && (btn == 1);
    const bool checkRight = (sel == 1 || sel == 2) && (btn == 2);

    auto now = std::chrono::steady_clock::now();

    // lazily create rateLimiter (constructor sets initial burst == rate)
    if (!rateLimiter) {
        double r = static_cast<double>(std::get<FloatValue>(maxCPS).value);
        rateLimiter = std::make_shared<RateLimiter>(r);
    } else {
        // keep limiter's rate in sync with the setting
        rateLimiter->setRate(static_cast<double>(std::get<FloatValue>(maxCPS).value));
    }

    if (checkLeft || checkRight) {
        bool allowed = rateLimiter->tryConsumeOne(now);
        if (!allowed) {
            // blocked by limiter: cancel event and do NOT record as an allowed click
            ev.setCancelled(true);
            return;
        }

        // allowed -> record as an "allowed click" so HUD shows the limited CPS
        Latite::get().getTimings().onAllowedClick(btn, true);
        return;
    }
}

