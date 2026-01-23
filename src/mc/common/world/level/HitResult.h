#pragma once
#include "util/LMath.h"
#include "mc/Util.h"

namespace SDK {
	enum struct HitType : int {
		BLOCK = 0,
		ENTITY = 1,
		AIR = 3
	};

	// Updated HitResult layout (matches observed runtime layout / Farial / MarioCST notes)
	// Offsets (important):
	//  - hitType @ 0x18
	//  - face    @ 0x1C
	//  - hitBlock@ 0x20
	//  - hitPos  @ 0x2C
	//  - entity  @ 0x38
	//  - isLiquid@ 0x4C
	//  - liquidFace @ 0x50
	//  - liquidBlock (BlockPos) @ 0x54
	//  - liquidPos (Vec3) @ 0x60
	//  - indirectHit @ 0x6C
	class HitResult {
	public:
		Vec3 start;            // 0x00
		Vec3 end;              // 0x0C
		HitType hitType;       // 0x18 (int)
		int32_t face;          // 0x1C (was incorrectly declared as int8_t previously)
		BlockPos hitBlock;     // 0x20
		Vec3 hitPos;           // 0x2C

		// additional fields observed in runtime
		void* entity;          // 0x38 (WeakEntityRef or pointer)
		// padding up to 0x4C
		bool isLiquid;         // 0x4C
		char pad_0x4D[3];      // 0x4D
		int32_t liquidFace;    // 0x50
		BlockPos liquidBlock;  // 0x54
		Vec3 liquidPos;        // 0x60
		bool indirectHit;      // 0x6C
		char pad_0x6D[3];      // 0x6D

		// keep CLASS_FIELD entries for downstream code that used them
		CLASS_FIELD(BlockPos, liquidBlock, 0x54);
		CLASS_FIELD(Vec3, liquidPos, 0x60);
	};
}