#include "pch.h"
#include "Trails.h"
#include <util/DrawUtil3D.h>
#include <mc/common/world/level/Level.h>
#include <algorithm>

Trails::Trails() : Module("Trails", LocalizeString::get("client.module.trails.name"),
                         LocalizeString::get("client.module.trails.desc"), GAME) {
    addSetting("renderThrough", LocalizeString::get("client.module.trails.renderThrough.name"), L"",
               renderThrough);
    addSetting("color", LocalizeString::get("client.module.trails.color.name"), L"", color);
    addSliderSetting("opacity", LocalizeString::get("client.module.trails.opacity.name"),
                     LocalizeString::get("client.module.trails.opacity.desc"), opacity, FloatValue(0.f), FloatValue(1.f), FloatValue(0.01f));
    addSliderSetting("sampleInterval", LocalizeString::get("client.module.trails.sampleInterval.name"),
                     LocalizeString::get("client.module.trails.sampleInterval.desc"), sampleInterval, FloatValue(0.01f), FloatValue(2.f), FloatValue(0.01f));
    addSliderSetting("maxPoints", LocalizeString::get("client.module.trails.maxPoints.name"),
                     LocalizeString::get("client.module.trails.maxPoints.desc"), maxPoints, IntValue(0), IntValue(5000), IntValue(1));
    addSetting("clear", LocalizeString::get("client.module.trails.clear.name"), LocalizeString::get("client.module.trails.clear.desc"), clearTrail);

    // fade settings
    addSetting("fade", LocalizeString::get("client.module.trails.fade.name"), LocalizeString::get("client.module.trails.fade.desc"), fade);
    addSliderSetting("tailOpacity", LocalizeString::get("client.module.trails.tailOpacity.name"),
                     LocalizeString::get("client.module.trails.tailOpacity.desc"), tailOpacity, FloatValue(0.f), FloatValue(1.f), FloatValue(0.01f));

    Eventing::get().listen<RenderLevelEvent, &Trails::onRenderLevel>(this);
}

static inline float dist2(const Vec3 &a, const Vec3 &b) {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    float dz = a.z - b.z;
    return dx * dx + dy * dy + dz * dz;
}

void Trails::onRenderLevel(RenderLevelEvent &event) {
    // one-shot clear from UI
    if (std::get<BoolValue>(clearTrail)) {
        m_points.clear();
        std::get<BoolValue>(clearTrail) = BoolValue(false);
    }

    // enforce maxPoints immediately so UI changes take effect without waiting for next sample
    {
        int maxP_now = std::get<IntValue>(maxPoints).value;
        if (maxP_now > 0) {
            while ((int)m_points.size() > maxP_now) m_points.pop_front();
        }
    }

    auto lp = SDK::ClientInstance::get()->getLocalPlayer();
    if (!lp) return;

    // interpolate player position for smoothness (getPos() is eye position)
    auto &posOld = lp->getPosOld();
    auto &pos = lp->getPos();
    float alpha = SDK::ClientInstance::get()->minecraft->timer->alpha;
    Vec3 interp = { std::lerp(posOld.x, pos.x, alpha), std::lerp(posOld.y, pos.y, alpha), std::lerp(posOld.z, pos.z, alpha) };

#ifdef IMGUI_VERSION
    float dt = ImGui::GetIO().DeltaTime;
#else
    float dt = 1.f / 60.f;
#endif

    float interval = std::max(0.001f, std::get<FloatValue>(sampleInterval).value);
    m_sampleTimer += dt;
    // convert eye pos -> foot pos (approximate)
    Vec3 foot = { interp.x, interp.y - 1.62f, interp.z };

    if (m_sampleTimer >= interval) {
        m_sampleTimer = 0.f;
        if (m_points.empty() || dist2(m_points.back(), foot) >= (kMinPointDist * kMinPointDist)) {
            int maxP = std::get<IntValue>(maxPoints).value;
            if (maxP > 0 && (int)m_points.size() >= maxP) {
                // drop oldest to make room for newest (keeps newest points)
                m_points.pop_front();
            }
            m_points.push_back(foot);
        }

        // ensure size (defensive)
        int maxP = std::get<IntValue>(maxPoints).value;
        if (maxP > 0) {
            while ((int)m_points.size() > maxP) m_points.pop_front();
        }
    }

    if (m_points.size() < 2) return;

    // choose material (match other visual modules)
    SDK::MaterialPtr *mat = std::get<BoolValue>(renderThrough) ? SDK::MaterialPtr::getUIColor()
                                                               : SDK::MaterialPtr::getSelectionOverlayMaterial();

    MCDrawUtil3D dc{SDK::ClientInstance::get()->levelRenderer, event.getScreenContext(), mat};

    // base color (we will modulate alpha per-segment when fading)
    float baseOpacity = std::get<FloatValue>(opacity).value;
    d2d::Color baseCol = d2d::Color(std::get<ColorValue>(color).getMainColor());

    bool useFade = std::get<BoolValue>(fade);
    float minTailA = std::clamp(std::get<FloatValue>(tailOpacity).value, 0.f, 1.f);
    size_t n = m_points.size();

    // draw consecutive line segments with optional linear fade (oldest -> minTailA, newest -> 1.0)
    for (size_t i = 0; i + 1 < n; ++i) {
        float t = (n <= 1) ? 1.f : static_cast<float>(i) / static_cast<float>(n - 1); // 0 = oldest, 1 = newest
        float segFactor = useFade ? std::lerp(minTailA, 1.f, t) : 1.f;
        d2d::Color segCol = baseCol.asAlpha(baseOpacity * segFactor);
        dc.drawLine(m_points[i], m_points[i + 1], segCol);
    }

    dc.flush();
} 
