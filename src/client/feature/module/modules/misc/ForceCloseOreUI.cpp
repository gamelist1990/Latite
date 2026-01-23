#include "pch.h"
#include "ForceCloseOreUI.h"
#include "client/event/events/RenderLayerEvent.h"
#include "client/screen/ScreenManager.h"
#include <mc/common/client/gui/controls/VisualTree.h>
#include <mc/common/client/gui/controls/UIControl.h>

ForceCloseOreUI::ForceCloseOreUI() : Module("ForceCloseOreUI", LocalizeString::get("client.module.forceCloseOreUI.name"),
                                        LocalizeString::get("client.module.forceCloseOreUI.desc"), GAME) {
    // Module is simple on/off — no extra settings shown to user

    listen<RenderLayerEvent>((EventListenerFunc)&ForceCloseOreUI::onRenderLayer);
}

void ForceCloseOreUI::onInit() {
    // nothing special
}


bool ForceCloseOreUI::matchRootName(const std::string &rootName) const {
    if (rootName.empty()) return false;
    std::string low = rootName;
    std::transform(low.begin(), low.end(), low.begin(), ::tolower);

    // conservative built-in matches for common container/ore UIs
    static const std::array<const char*, 4> defaults = {"ore", "container", "furnace", "anvil"};
    for (auto k : defaults) if (low.find(k) != std::string::npos) return true;
    return false;
}

void ForceCloseOreUI::onRenderLayer(Event& evGeneric) {
    // Module is ON/OFF only — use the Module enabled flag
    if (!this->isEnabled()) return;

    auto &ev = reinterpret_cast<RenderLayerEvent&>(evGeneric);
    SDK::ScreenView* view = ev.getScreenView();
    if (!view || !view->visualTree || !view->visualTree->rootControl) return;

    const std::string &rootName = view->visualTree->rootControl->name;
    if (!matchRootName(rootName)) return;

    // debounce to avoid spamming exitCurrentScreen during a single UI lifetime
    static auto lastClose = std::chrono::steady_clock::time_point{};
    auto now = std::chrono::steady_clock::now();
    if (now - lastClose < std::chrono::milliseconds(40)) return;
    lastClose = now;

    // close the screen to force old UI behavior (keep server-side state intact)
    Latite::getScreenManager().exitCurrentScreen();
    Latite::getClientMessageQueue().display(LocalizeString::get("client.module.forceCloseOreUI.forcedMessage"));
}

