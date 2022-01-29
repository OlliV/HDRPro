//------------------------------------------------------------------------
// Copyright(c) 2021-2022 Olli Vanhoja.
//------------------------------------------------------------------------

#pragma once

#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/vst/vsttypes.h"

namespace MyVst {
static const Steinberg::FUID kHDRProProcessorUID (0x95C42BD8, 0x75EF5DFB, 0x8D8989C4, 0x1A898E0D);
static const Steinberg::FUID kHDRProControllerUID (0x53E48F5E, 0xA55B5A7B, 0x84811561, 0x6E1499C8);

#define HDRProVST3Category "Fx"
}
