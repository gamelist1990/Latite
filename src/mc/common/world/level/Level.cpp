#include "pch.h"
#include "Level.h"
#include <mc/common/world/level/HitResult.h>

void SDK::Level::playSoundEvent(std::string const &text, Vec3 const &pos, float vol, float pitch)
{
	static int index = mvGetOffset<0xB2, 0xB3, 0xB2, 0xB2, 0xA2, 0xA2, 0xA2, 0xA3, 0xA3, 0xBD, 0xC9>();
	memory::callVirtual<void>(this, 0xB6, text, pos, vol, pitch);
}

std::vector<SDK::Actor *> SDK::Level::getRuntimeActorList()
{
	std::vector<Actor *> list;
	static int index = mvGetOffset<0x13C, 0x13B, 0x13A, 0x139, 0x135, 0x135, 0x136, 0x134, 0x132, 0x134, 0x117, 0x117, 0x116, 0x125, 0x125, 0x12D, 0x13C>();

	// TODO: this might return a vector too?
	memory::callVirtual<void, std::vector<Actor *> &>(this, index, list);
	return list;
}

std::unordered_map<UUID, SDK::PlayerListEntry> *SDK::Level::getPlayerList()
{
	if (internalVers >= SDK::V1_21_40)
	{
		return *reinterpret_cast<std::unordered_map<UUID, SDK::PlayerListEntry> **>(reinterpret_cast<uintptr_t>(this) + 0x4E0);
	}

	static int index = SDK::mvGetOffset<0x112, 0x111, 0x120, 0x120, 0x128, 0x137>();
	return memory::callVirtual<std::unordered_map<UUID, SDK::PlayerListEntry> *>(this, index);
}

SDK::HitResult *SDK::Level::getHitResult()
{
	// Prefer direct member read (fast and stable when offset is correct). If that fails,
	// fall back to the virtual-function-based accessor for compatibility.
	// NOTE: avoid dereferencing shared_ptr without a check to prevent UB/crashes.
	auto &maybe = hat::member_at<std::shared_ptr<HitResult>>(this, 0x1E8);
	if (maybe)
	{
		SDK::HitResult *hr = maybe.get();
		if (hr)
		{
			// cheap sanity: start Vec3 should be finite and within a sane range
			if (std::isfinite(hr->start.x) && std::fabs(hr->start.x) < 1e6f)
				return hr;
		}
	}

	// Fallback to the existing vfunc (keeps compatibility across versions).
	static int index = mvGetOffset<0x146, 0x145, 0x144, 0x143, 0x13F, 0x13F, 0x140, 0x13E, 0x13C, 0x13E, 0x121, 0x121, 0x120, 0x12E, 0x12E, 0x139, 0x148>();
	HitResult *hr = memory::callVirtual<HitResult *>(this, index);
	static bool loggedFallback = false;
	if (!loggedFallback)
	{
		Logger::Info("Level::getHitResult: member access failed or not sane; using vfunc index=0x{:X}", index);
		loggedFallback = true;
	}
	return hr;
}

SDK::HitResult *SDK::Level::getLiquidHitResult()
{
	static int index = mvGetOffset<0x147, 0x146, 0x145, 0x144, 0x140, 0x140, 0x141, 0x13F, 0x13D, 0x13F, 0x122, 0x122, 0x121, 0x12F, 0x12F, 0x13A, 0x149>();
	return reinterpret_cast<SDK::HitResult *>(memory::callVirtual<uintptr_t>(this, index)) /*sizeof hitResult (0x60) / 8*/;
}

bool SDK::Level::isClientSide()
{
	return memory::callVirtual<bool>(this, SDK::mvGetOffset<0x135, 0x134, 0x133, 0x132, 0x12E, 0x12E, 0x12F, 0x12D, 0x12B, 0x12C, 0x111, 0x111, 0x110, 0x11F, 0x11F, 0x12B, 0x127>());
}

const std::string &SDK::Level::getLevelName()
{
	if (internalVers >= V1_21_20)
	{
		return levelData->levelName;
	}
	return name;
}
