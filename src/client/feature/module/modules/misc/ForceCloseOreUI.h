#pragma once

#include <client/feature/module/Module.h>

class ForceCloseOreUI : public Module {
public:
    ForceCloseOreUI();
    virtual ~ForceCloseOreUI() = default;

    void onInit() override;

private:
    // On/Off only: no extra user-facing settings
    void onRenderLayer(Event& ev);
    bool matchRootName(const std::string &rootName) const;
};
