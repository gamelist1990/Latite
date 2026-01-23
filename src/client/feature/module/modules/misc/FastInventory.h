#pragma once
#include "../../Module.h"
#include <queue>
#include <mutex>

class FastInventory : public Module {
public:
    FastInventory();

    void onSendPacket(Event& ev);
    void onPacketReceive(Event& ev);
    void onTick(Event& ev);
    void onRenderLayer(Event& ev);

private:
    // settings
    ValueType restricted = BoolValue(false);

    // runtime
    bool inventoryOpen = false;                // client thinks inventory is open
    bool inventoryOpenServerside = false;      // server thinks inventory is open
    bool sendingClosePacket = false;
    int desyncTicks = 0;
    static constexpr int kDesyncTicksLimit = 10;

    std::queue<int> containerQueue; // container ids queued from server
    std::mutex queueMutex;
};