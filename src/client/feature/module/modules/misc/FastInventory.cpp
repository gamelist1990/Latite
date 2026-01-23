#include "pch.h"
#include "FastInventory.h"
#include "client/Latite.h"
#include "client/screen/ScreenManager.h"
#include <mc/common/client/gui/controls/VisualTree.h>
#include <mc/common/client/gui/controls/UIControl.h>
#include <mc/common/client/gui/ScreenView.h>

FastInventory::FastInventory() : Module("FastInventory", LocalizeString::get("client.module.fastInventory.name"),
                                     LocalizeString::get("client.module.fastInventory.desc"), GAME) {
    addSetting("restricted", LocalizeString::get("client.module.fastInventory.restricted.name"),
               LocalizeString::get("client.module.fastInventory.restricted.desc"), restricted);

    // listen for packets / tick / screen changes
    listen<SendPacketEvent>((EventListenerFunc)&FastInventory::onSendPacket);
    listen<PacketReceiveEvent>((EventListenerFunc)&FastInventory::onPacketReceive);
    listen<TickEvent>((EventListenerFunc)&FastInventory::onTick);
    listen<RenderLayerEvent>((EventListenerFunc)&FastInventory::onRenderLayer);
}

void FastInventory::onSendPacket(Event& evG) {
    auto& ev = reinterpret_cast<SendPacketEvent&>(evG);
    if (!this->isEnabled()) return;
    if (!SDK::ClientInstance::get()->getLocalPlayer()) return;
    if (std::get<BoolValue>(restricted)) return;

    SDK::Packet* pkt = ev.getPacket();
    if (!pkt) return;

    // Defensive: use packet name so we don't need concrete packet types
    std::string name = pkt->getName();

    // When player attempts to close inventory client-side, cancel the outgoing close so server
    // still thinks inventory is open (we keep a virtual open state). This enables faster
    // client-side interactions while minimizing server sync.
    if (name == "ContainerClose" || name == "container_close") {
        if (!inventoryOpen) return; // nothing to do
        // prevent sending the close to server so server-side stays open
        ev.setCancelled(true);
        return;
    }

    // If player explicitly opens their own inventory (client -> server), capture that state.
    if (name == "Interact" || name == "interact_packet") {
        // best-effort: if client is opening inventory, mark virtual-open and close the screen
        // real detection of open-screen is handled in onRenderLayer (more reliable)
    }
}

void FastInventory::onPacketReceive(Event& evG) {
    auto& ev = reinterpret_cast<PacketReceiveEvent&>(evG);
    if (!this->isEnabled()) return;
    if (!SDK::ClientInstance::get()->getLocalPlayer()) return;
    if (std::get<BoolValue>(restricted)) return;

    SDK::Packet* pkt = ev.getPacket();
    if (!pkt) return;

    std::string name = pkt->getName();

    // Server opened a container for us -> keep client closed but record server-side open
    if (name == "ContainerOpen" || name == "container_open") {
        inventoryOpenServerside = true;

        // If client UI is not opened, immediately close client (defensive) so we can operate
        // without showing the vanilla UI. We don't attempt to construct or send packets here;
        // we just keep internal state and prevent client-side close packets from reaching server.
        if (!inventoryOpen) {
            // try to immediately close any client UI that might pop up
            Latite::getScreenManager().exitCurrentScreen();
        } else {
            // If client already had a virtual-open, we could enqueue container id — skip (no pkt cast)
        }

        // we don't cancel receive events here (may break game logic) but we've reacted to it.
        return;
    }

    if (name == "ContainerClose" || name == "container_close") {
        // server closed our container
        inventoryOpenServerside = false;
    }
}

void FastInventory::onTick(Event& /*evG*/) {
    if (!this->isEnabled()) return;
    if (!SDK::ClientInstance::get()->getLocalPlayer()) return;
    if (std::get<BoolValue>(restricted)) return;

    // Keep desync detection: if client/server disagree on "open" state, count ticks and reset
    if (inventoryOpen != inventoryOpenServerside) {
        desyncTicks++;
    } else {
        desyncTicks = 0;
    }

    if (desyncTicks > kDesyncTicksLimit) {
        // Force-close client and clear queue to recover
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            while (!containerQueue.empty()) containerQueue.pop();
        }
        inventoryOpen = false;
        inventoryOpenServerside = false;
        desyncTicks = 0;
        Latite::getScreenManager().exitCurrentScreen();
    }
}

void FastInventory::onRenderLayer(Event& evG) {
    auto& ev = reinterpret_cast<RenderLayerEvent&>(evG);
    if (!this->isEnabled()) return;
    if (std::get<BoolValue>(restricted)) return;

    // Detect the vanilla inventory screen when it becomes active and immediately close the client
    // UI while keeping an internal "open" flag so player can continue fast interactions.
    SDK::ScreenView* view = ev.getScreenView();
    if (!view || !view->visualTree || !view->visualTree->rootControl) return;

    const std::string& rootName = view->visualTree->rootControl->name;
    if (rootName == "inventory_screen") {
        // mark virtual open and immediately close the visible UI
        inventoryOpen = true;
        Latite::getScreenManager().exitCurrentScreen();
    }
}
