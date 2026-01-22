#include "pch.h"
#include "MotionBlur.h"
#include "client/render/Renderer.h"
#include "client/Latite.h"

#include "client/event/events/RendererInitEvent.h"

MotionBlur::MotionBlur() : Module("MotionBlur", LocalizeString::get("client.module.motionBlur.name"),
    LocalizeString::get("client.module.motionBlur.desc"), GAME) {
    addSetting("usePixelAverage", LocalizeString::get("client.module.motionBlur.usePixelAverage.name"), LocalizeString::get("client.module.motionBlur.usePixelAverage.desc"), usePixelAverage);
    addSliderSetting("intensity", LocalizeString::get("client.module.motionBlur.intensity.name"), LocalizeString::get("client.module.motionBlur.intensity.desc"), intensity,
        FloatValue(0.f), FloatValue(20.f), FloatValue(1.f), "usePixelAverage"_istrue);
    addSliderSetting("opacity", LocalizeString::get("client.module.motionBlur.opacity.name"), LocalizeString::get("client.module.motionBlur.opacity.desc"), opacity,
        FloatValue(2.f), FloatValue(16.f), FloatValue(1.f));

    listen<RendererCleanupEvent>(&MotionBlur::onCleanup);
    listen<RenderOverlayEvent>(&MotionBlur::onRenderOverlay, true, 100);
    listen<RendererInitEvent>(&MotionBlur::onRendererInit, true);
}

void MotionBlur::clearFrames() {
    SafeRelease(&m_previousFrameBitmap);
    for (auto& frame : m_frameHistory) {
        SafeRelease(&frame);
    }
	
    m_frameHistory.clear();
    m_frameWeights.clear();
}

void MotionBlur::updateFrameWeights() {
    size_t frameCount = m_frameHistory.size();
    if (frameCount == 0) return;

    m_frameWeights.resize(frameCount);

    // ガウシアン分布に基づいた重み付け
    // 新しいフレーム（最後）ほど高い重みを持つ
    float sigma = frameCount / 2.5f;
    float totalWeight = 0.f;

    for (size_t i = 0; i < frameCount; ++i) {
        float distance = static_cast<float>(frameCount - 1 - i);
        float weight = expf(-(distance * distance) / (2.f * sigma * sigma));
        m_frameWeights[i] = weight;
        totalWeight += weight;
    }

    // 正規化
    if (totalWeight > 0.f) {
        for (auto& weight : m_frameWeights) {
            weight /= totalWeight;
        }
    }
}

void MotionBlur::onEnable() {
    clearFrames();
}

void MotionBlur::onDisable() {
    clearFrames();
}

void MotionBlur::onCleanup(Event&) {
    clearFrames();
}

void MotionBlur::onRendererInit(Event&) {
    clearFrames();
}

void MotionBlur::onRenderOverlay(Event& genericEv) {
    if (!this->isEnabled()) {
        return;
    }

    bool currentModeIsPixelAverage = std::get<BoolValue>(usePixelAverage);
    if (currentModeIsPixelAverage != m_lastModeWasPixelAverage) {
        clearFrames();
        m_lastModeWasPixelAverage = currentModeIsPixelAverage;
    }

    RenderOverlayEvent& ev = reinterpret_cast<RenderOverlayEvent&>(genericEv);

    ID2D1DeviceContext* ctx = ev.getDeviceContext();
    Renderer* renderer = &Latite::getRenderer();
    D2D1_SIZE_F screenSize = renderer->getScreenSize();
    D2D1_RECT_F rc = D2D1::RectF(0.f, 0.f, screenSize.width, screenSize.height);
    float opacityValue = std::get<FloatValue>(opacity);

    if (currentModeIsPixelAverage) {
        // 現在のフレームを履歴に追加
        ID2D1Bitmap1* currentFrame = renderer->copyCurrentBitmap();
        if (currentFrame) {
            m_frameHistory.push_back(currentFrame);
        }

        size_t intensityValue = static_cast<size_t>(std::get<FloatValue>(intensity).getInt());
        
        // 強度が変更された場合、重みを再計算
        if (intensityValue != m_lastIntensity) {
            m_lastIntensity = intensityValue;
            updateFrameWeights();
        }

        // フレーム履歴を制限
        while (m_frameHistory.size() > intensityValue) {
            SafeRelease(&m_frameHistory.front());
            m_frameHistory.erase(m_frameHistory.begin());
        }

        // 重みを再計算（フレーム数が変わった場合）
        if (m_frameWeights.size() != m_frameHistory.size()) {
            updateFrameWeights();
        }

        // ガウシアン重み付けでフレームを描画
        if (!m_frameHistory.empty() && opacityValue > 0) {
            float maxOpacity = opacityValue / 10.f;
            if (maxOpacity > 1.f) maxOpacity = 1.f;

            for (size_t i = 0; i < m_frameHistory.size(); ++i) {
                ID2D1Bitmap1* frame = m_frameHistory[i];
                if (frame && i < m_frameWeights.size()) {
                    float finalOpacity = maxOpacity * m_frameWeights[i];
                    ctx->DrawBitmap(frame, &rc, finalOpacity);
                }
            }
        }
    } else {
        // シングルフレームモード：より滑らかなブレンド
        if (m_previousFrameBitmap) {
            float blendOpacity = opacityValue / 12.f;
            if (blendOpacity > 1.f) blendOpacity = 1.f;
            ctx->DrawBitmap(m_previousFrameBitmap, &rc, blendOpacity);
        }

        SafeRelease(&m_previousFrameBitmap);
        m_previousFrameBitmap = renderer->copyCurrentBitmap();
    }
}