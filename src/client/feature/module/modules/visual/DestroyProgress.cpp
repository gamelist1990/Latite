#include "pch.h"
#include "DestroyProgress.h"
#include <util/DrawUtil3D.h>
#include <mc/common/world/level/HitResult.h>
#include <cmath>


DestroyProgress::DestroyProgress() : Module("DestroyProgress", LocalizeString::get("client.module.destroyProgress.name"),
                                          LocalizeString::get("client.module.destroyProgress.desc"), GAME) {
    addSetting("renderThrough", LocalizeString::get("client.module.destroyProgress.renderThrough.name"), L"",
               renderThrough);
    addSetting("transparent", LocalizeString::get("client.module.destroyProgress.transparent.name"), L"",
               transparent);
    addSetting("filled", LocalizeString::get("client.module.destroyProgress.filled.name"), L"",
               filled);
    addSetting("color", LocalizeString::get("client.module.destroyProgress.color.name"), L"",
               color);
    addSliderSetting("opacity", LocalizeString::get("client.module.destroyProgress.opacity.name"),
                     LocalizeString::get("client.module.destroyProgress.opacity.desc"), opacity, FloatValue(0.f),
                     FloatValue(1.f), FloatValue(0.85f));
    addSliderSetting("speed", LocalizeString::get("client.module.destroyProgress.speed.name"),
                     LocalizeString::get("client.module.destroyProgress.speed.desc"), speed, FloatValue(1.f),
                     FloatValue(60.f), FloatValue(12.f));

    Eventing::get().listen<RenderLevelEvent, &DestroyProgress::onRenderLevel>(this);
}

void DestroyProgress::onRenderLevel(RenderLevelEvent &event) {
    auto lp = SDK::ClientInstance::get()->getLocalPlayer();
    if (!lp) return;
    if (SDK::ClientInstance::get()->minecraftGame->isCursorGrabbed()) return;

    auto gm = lp->gameMode;
    if (!gm) return;

    auto hr = SDK::ClientInstance::get()->minecraft->getLevel()->getHitResult();
    if (!hr || hr->hitType != SDK::HitType::BLOCK) return;

    float targetProg = gm->breakProgress; // [0..1]
    BlockPos bp = hr->hitBlock;

    // reset animation when switching target block
    if (!(bp.x == m_lastPos.x && bp.y == m_lastPos.y && bp.z == m_lastPos.z)) {
        m_animProgress = 0.f;
        m_popTimer = 0.f;
        m_lastPos = bp;
    }

    // animate towards target progress
#ifdef IMGUI_VERSION
    float dt = ImGui::GetIO().DeltaTime;
#else
    // Fallback when ImGui isn't available in the include path (keeps behaviour stable)
    float dt = 1.f / 60.f;
#endif
    float lerpSpeed = std::get<FloatValue>(speed);
    m_animProgress = std::lerp(m_animProgress, targetProg, LatiteMath::aequals(dt, 0.f) ? 1.f : (dt * lerpSpeed));
    m_animProgress = std::clamp(m_animProgress, 0.f, 1.25f);

    // when finished, trigger a short pop animation
    if (targetProg >= 1.f && m_popTimer <= 0.f) {
        m_popTimer = 0.25f; // quarter-second pop
    }
    if (m_popTimer > 0.f) {
        m_popTimer -= dt;
    }

    // nothing to draw
    if (m_animProgress <= 0.0001f) return;

    // build AABB — scale from centre of block
    Vec3 center = { static_cast<float>(bp.x) + 0.5f, static_cast<float>(bp.y) + 0.5f, static_cast<float>(bp.z) + 0.5f };

    // apply pop: small overshoot when recently completed
    float popFactor = 1.f;
    if (m_popTimer > 0.f) {
        float t = 1.f - (m_popTimer / 0.25f); // 0..1
        popFactor = 1.f + std::sin(t * 3.14159265f) * 0.15f; // peak ~+15%
    }

    float scale = m_animProgress * popFactor;
    scale = std::clamp(scale, 0.001f, 1.25f);

    Vec3 half = { scale * 0.5f, scale * 0.5f, scale * 0.5f };
    AABB bb{ center - half, center + half };

    // choose material (renderThrough / transparent)
    SDK::MaterialPtr* mat = std::get<BoolValue>(renderThrough) ? SDK::MaterialPtr::getUIColor()
        : (std::get<BoolValue>(transparent) ? SDK::MaterialPtr::getSelectionOverlayMaterial() : SDK::MaterialPtr::getSelectionBoxMaterial());

    MCDrawUtil3D dc{ SDK::ClientInstance::get()->levelRenderer, event.getScreenContext(), mat };

    auto stored = std::get<ColorValue>(color).getMainColor();
    d2d::Color col = d2d::Color(stored).asAlpha(std::get<FloatValue>(opacity));

    if (std::get<BoolValue>(filled)) {
        // draw 6 faces (filled) + outline
        dc.fillQuad({ bb.lower.x, bb.lower.y, bb.lower.z }, { bb.higher.x, bb.lower.y, bb.lower.z },
                    { bb.higher.x, bb.lower.y, bb.higher.z }, { bb.lower.x, bb.lower.y, bb.higher.z }, col);
        dc.fillQuad({ bb.lower.x, bb.higher.y, bb.lower.z }, { bb.higher.x, bb.higher.y, bb.lower.z },
                    { bb.higher.x, bb.higher.y, bb.higher.z }, { bb.lower.x, bb.higher.y, bb.higher.z }, col);
        // back face (z = higher) — fixed vertex order (was mixing top/bottom vertices and produced a twisted quad)
        dc.fillQuad({ bb.lower.x, bb.lower.y, bb.higher.z }, { bb.higher.x, bb.lower.y, bb.higher.z },
                    { bb.higher.x, bb.higher.y, bb.higher.z }, { bb.lower.x, bb.higher.y, bb.higher.z }, col);
        dc.fillQuad({ bb.lower.x, bb.lower.y, bb.lower.z }, { bb.higher.x, bb.lower.y, bb.lower.z },
                    { bb.higher.x, bb.higher.y, bb.lower.z }, { bb.lower.x, bb.higher.y, bb.lower.z }, col);
        dc.fillQuad({ bb.lower.x, bb.lower.y, bb.lower.z }, { bb.lower.x, bb.lower.y, bb.higher.z },
                    { bb.lower.x, bb.higher.y, bb.higher.z }, { bb.lower.x, bb.higher.y, bb.lower.z }, col);
        dc.fillQuad({ bb.higher.x, bb.lower.y, bb.lower.z }, { bb.higher.x, bb.lower.y, bb.higher.z },
                    { bb.higher.x, bb.higher.y, bb.higher.z }, { bb.higher.x, bb.higher.y, bb.lower.z }, col);

        // outline (slightly stronger alpha)
        d2d::Color outlineCol = d2d::Color(stored).asAlpha(std::min(1.f, std::get<FloatValue>(opacity) + 0.15f));
        dc.drawBox(bb, outlineCol);
    }
    else {
        dc.drawBox(bb, col);
    }

    dc.flush();
}
