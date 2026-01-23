#include "pch.h"
#include "Hitping.h"
#include <mc/common/network/packet/ActorEventPacket.h>
#include "util/Util.h"

Hitping::Hitping() : Module("Hitping", LocalizeString::get("client.module.hitping.name"),
                          LocalizeString::get("client.module.hitping.desc"), GAME, nokeybind) {
    // enum: 0 = All, 1 = Players
    targetMode.addEntry(EnumEntry(0, L"All", L"All entities"));
    targetMode.addEntry(EnumEntry(1, L"Players", L"Players only"));
    std::get<EnumValue>(*targetMode.getValue()) = 1; // default: players (user preference)
    // ensure settings have sane defaults
    std::get<TextValue>(soundId).str = L"random.pop";
    std::get<FloatValue>(volume).value = 1.f;
    std::get<FloatValue>(pitch).value = 1.f;

    addSetting("soundSetting", LocalizeString::get("client.module.hitping.sound.name"),
               LocalizeString::get("client.module.hitping.sound.desc"), soundId);
    addSliderSetting("volumeSetting", LocalizeString::get("client.module.hitping.volume.name"),
                     LocalizeString::get("client.module.hitping.volume.desc"), volume, FloatValue(0.f), FloatValue(2.f), FloatValue(0.05f));
    addSliderSetting("pitchSetting", LocalizeString::get("client.module.hitping.pitch.name"),
                     LocalizeString::get("client.module.hitping.pitch.desc"), pitch, FloatValue(0.5f), FloatValue(2.f), FloatValue(0.05f));

    addEnumSetting("targetSetting", LocalizeString::get("client.module.hitping.target.name"),
                   LocalizeString::get("client.module.hitping.target.desc"), targetMode);

    addSetting("cooldownEnabledSetting", LocalizeString::get("client.module.hitping.cooldownEnabled.name"),
               LocalizeString::get("client.module.hitping.cooldownEnabled.desc"), cooldownEnabled);
    addSliderSetting("cooldownMsSetting", LocalizeString::get("client.module.hitping.cooldownMs.name"),
                     LocalizeString::get("client.module.hitping.cooldownMs.desc"), cooldownMs, FloatValue(0.f), FloatValue(2000.f), FloatValue(10.f), "cooldownEnabledSetting"_istrue);

    listen<AttackEvent>(static_cast<EventListenerFunc>(&Hitping::onAttack));
    listen<PacketReceiveEvent>(static_cast<EventListenerFunc>(&Hitping::onPacketReceive));
    // no-op (keep default Module enabled flag handling)
}

void Hitping::onAttack(Event& evG) {
    auto& ev = reinterpret_cast<AttackEvent&>(evG);
    SDK::Actor* ent = ev.getActor();
    if (!ent) return;

    // Respect target setting
    const int selected = targetMode.getSelectedKey();
    if (selected == 1 && !ent->isPlayer()) return; // players-only

    lastRuntimeId = ent->getRuntimeID();
    lastWasPlayer = ent->isPlayer();
    hasHit = true;
}

void Hitping::onPacketReceive(Event& evG) {
    auto& ev = reinterpret_cast<PacketReceiveEvent&>(evG);
    auto pkt = ev.getPacket();

    if (!hasHit) return;

    if (pkt->getID() == SDK::PacketID::ACTOR_EVENT) {
        auto actorEvent = static_cast<SDK::ActorEventPacket*>(pkt);
        if (actorEvent->eventID == SDK::ActorEventID::HURT_ANIMATION && actorEvent->runtimeID == lastRuntimeId) {
            // cooldown
            auto now = std::chrono::steady_clock::now();
            bool ok = true;
            if (std::get<BoolValue>(cooldownEnabled)) {
                auto ms = static_cast<int>(std::get<FloatValue>(cooldownMs).value);
                if (now - lastPingTime < std::chrono::milliseconds(ms)) ok = false;
            }

            if (ok) {
                // play sound (use util helper which routes to level playSoundEvent)
                std::string sid = util::WStrToStr(std::get<TextValue>(soundId).str);
                float vol = std::get<FloatValue>(volume).value;
                float pit = std::get<FloatValue>(pitch).value;
                util::PlaySoundUI(sid, vol, pit);
                lastPingTime = now;
            }

            hasHit = false;
        }
    }
}
