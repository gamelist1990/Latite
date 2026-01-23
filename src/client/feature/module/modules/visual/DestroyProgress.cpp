#include "pch.h"
#include "DestroyProgress.h"
#include <util/DrawUtil3D.h>
#include <mc/common/world/level/HitResult.h>
#include <cmath>

static constexpr float maxY = 320.f;
static constexpr float minY = -64.f;




namespace
{
    void renderFace(MCDrawUtil3D &dc, float x1, float z1, float x2, float z2, d2d::Color color)
    {
        dc.drawLine({x1, minY, z1}, {x1, maxY, z1}, color);

        float mMaxY = maxY;
        float mMinY = minY;

        for (float y = mMinY + 2.f; y <= mMaxY; y += 2.f)
        {
            dc.drawLine({x1, y, z1}, {x2, y, z2}, static_cast<int>(y) % 16 == 0 ? d2d::Color(0.f, 0.f, 1.f, color.a) : d2d::Color(1.f, 1.f, 0.f, color.a));
        }

        if (!LatiteMath::aequals(x1, x2))
        {
            // X mode
            float myDiff = (x2 - x1) / 8.f;

            if (myDiff > 0.f)
            {
                for (float myX = x1 + myDiff; myX < x2; myX += myDiff)
                {
                    dc.drawLine({myX, minY, z1}, {myX, maxY, z1}, d2d::Color(1.f, 1.f, 0.f, color.a));
                }
            }
            else
            {
                for (float myX = x1 + myDiff; myX > x2; myX += myDiff)
                {
                    dc.drawLine({myX, minY, z1}, {myX, maxY, z1}, d2d::Color(1.f, 1.f, 0.f, color.a));
                }
            }
        }
        else
        {
            float myDiff = (z2 - z1) / 8.f;

            if (myDiff > 0.f)
            {
                for (float myZ = z1 + myDiff; myZ < z2; myZ += myDiff)
                {
                    dc.drawLine({x1, minY, myZ}, {x1, maxY, myZ}, d2d::Color(1.f, 1.f, 0.f, color.a));
                }
            }
            else
            {
                for (float myZ = z1 + myDiff; myZ > z2; myZ += myDiff)
                {
                    dc.drawLine({x1, minY, myZ}, {x1, maxY, myZ}, d2d::Color(1.f, 1.f, 0.f, color.a));
                }
            }
            // Z mode
        }
    }

    void renderBlockBorder(MCDrawUtil3D &dc, float minX, float minZ, float scale, d2d::Color color)
    {
        renderFace(dc, minX, minZ, minX + scale, minZ, color);
        renderFace(dc, minX + scale, minZ, minX + scale, minZ + scale, color);
        renderFace(dc, minX + scale, minZ + scale, minX, minZ + scale, color);
        renderFace(dc, minX, minZ + scale, minX, minZ, color);
    }
}

// Helper: create scaled block AABB and draw (outline + optional filled faces). Matches BlockOutline visuals.
static AABB makeScaledBlockAABB(const BlockPos &bp, float scale) {
    const Vec3 center{bp.x + 0.5f, bp.y + 0.5f, bp.z + 0.5f};
    const float half = 0.5f * scale; // scale==1.0 -> full block
    return AABB{Vec3{center.x - half, center.y - half, center.z - half}, Vec3{center.x + half, center.y + half, center.z + half}};
}

static void drawScaledBlockBox(MCDrawUtil3D &dc, const BlockPos &bp, float scale, d2d::Color const &col, bool filled) {
    AABB bb = makeScaledBlockAABB(bp, scale);

    if (filled) {
        const auto &l = bb.lower;
        const auto &h = bb.higher;
        // top
        dc.fillQuad({l.x, h.y, l.z}, {h.x, h.y, l.z}, {h.x, h.y, h.z}, {l.x, h.y, h.z}, col);
        // bottom
        dc.fillQuad({l.x, l.y, l.z}, {h.x, l.y, l.z}, {h.x, l.y, h.z}, {l.x, l.y, h.z}, col);
        // east
        dc.fillQuad({h.x, l.y, l.z}, {h.x, h.y, l.z}, {h.x, h.y, h.z}, {h.x, l.y, h.z}, col);
        // west
        dc.fillQuad({l.x, l.y, l.z}, {l.x, h.y, l.z}, {l.x, h.y, h.z}, {l.x, l.y, h.z}, col);
        // south
        dc.fillQuad({l.x, l.y, h.z}, {l.x, h.y, h.z}, {h.x, h.y, h.z}, {h.x, l.y, h.z}, col);
        // north
        dc.fillQuad({l.x, l.y, l.z}, {l.x, h.y, l.z}, {h.x, h.y, l.z}, {h.x, l.y, l.z}, col);
    }

    // outline (matches BlockOutline style)
    dc.drawBox(bb, col);
}

DestroyProgress::DestroyProgress() : Module("DestroyProgress", LocalizeString::get("client.module.destroyProgress.name"),
                                            LocalizeString::get("client.module.destroyProgress.desc"), GAME)
{
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


void DestroyProgress::onRenderLevel(RenderLevelEvent &event)
{

    auto lp = SDK::ClientInstance::get()->getLocalPlayer();
    if (!lp)
    {
        return;
    }

    auto gm = lp->gameMode;
    if (!gm)
    {
        return;
    }

    float targetProg = gm->breakProgress;
    auto hr = SDK::ClientInstance::get()->minecraft->getLevel()->getHitResult();

    // ブロック破壊進行度が0なら座標もリセットしてreturn
    if (targetProg == 0.f)
    {
        m_lastPos = BlockPos{INT_MIN, INT_MIN, INT_MIN};
        return;
    }

    BlockPos oldPos = m_lastPos;
    if (hr && hr->hitType == SDK::HitType::BLOCK)
    {
        m_lastPos = hr->hitBlock;
    }

    BlockPos bp = m_lastPos;
    if (bp.x == INT_MIN && bp.y == INT_MIN && bp.z == INT_MIN)
    {
        return;
    }

    if (!(oldPos.x == bp.x && oldPos.y == bp.y && oldPos.z == bp.z))
    {
        m_animProgress = 0.f;
        m_popTimer = 0.f;
    }

#ifdef IMGUI_VERSION
    float dt = ImGui::GetIO().DeltaTime;
#else
    float dt = 1.f / 60.f;
#endif
    float lerpSpeed = std::get<FloatValue>(speed);
    m_animProgress = std::lerp(m_animProgress, targetProg, LatiteMath::aequals(dt, 0.f) ? 1.f : (dt * lerpSpeed));
    m_animProgress = std::clamp(m_animProgress, 0.f, 1.25f);

    if (targetProg >= 1.f && m_popTimer <= 0.f)
    {
        m_popTimer = 0.25f; 
    }
    if (m_popTimer > 0.f)
    {
        m_popTimer -= dt;
    }

    if (m_animProgress <= 0.0001f)
    {
        return;
    }

    SDK::MaterialPtr *mat = std::get<BoolValue>(renderThrough) ? SDK::MaterialPtr::getUIColor()
                                                               : (std::get<BoolValue>(transparent) ? SDK::MaterialPtr::getSelectionOverlayMaterial() : SDK::MaterialPtr::getSelectionBoxMaterial());

    MCDrawUtil3D dc{SDK::ClientInstance::get()->levelRenderer, event.getScreenContext(), mat};

    auto stored = std::get<ColorValue>(color).getMainColor();
    d2d::Color col = d2d::Color(stored).asAlpha(std::get<FloatValue>(opacity));

    float scale = m_animProgress * (1.f + (m_popTimer > 0.f ? std::sin((1.f - m_popTimer / 0.25f) * 3.14159265f) * 0.15f : 0.f));
    scale = std::clamp(scale, 0.001f, 1.25f);
    drawScaledBlockBox(dc, bp, scale, col, std::get<BoolValue>(filled));

    dc.flush();
}
