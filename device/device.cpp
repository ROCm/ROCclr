/* Copyright (c) 2008 - 2022 Advanced Micro Devices, Inc.

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE. */

#include "device/device.hpp"
#include "thread/monitor.hpp"
#include "utils/options.hpp"
#include "comgrctx.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstring>

#if defined(WITH_HSA_DEVICE)
#include "device/rocm/rocdevice.hpp"
extern amd::AppProfile* rocCreateAppProfile();
#endif

#if defined(WITH_PAL_DEVICE)
// namespace pal {
extern bool PalDeviceLoad();
extern void PalDeviceUnload();
//}
#endif  // WITH_PAL_DEVICE

#include "platform/runtime.hpp"
#include "platform/program.hpp"
#include "thread/monitor.hpp"
#include "amdocl/cl_common.hpp"
#include "utils/options.hpp"
#include "utils/versions.hpp"  // AMD_PLATFORM_INFO

#if defined(HAVE_BLOWFISH_H)
#include "blowfish/oclcrypt.hpp"
#endif

#if defined(WITH_COMPILER_LIB)
#include "utils/bif_section_labels.hpp"
#include "utils/libUtils.h"
#include "spirv/spirvUtils.h"
#endif

#include <vector>
#include <string>
#include <cstring>
#include <cstdio>
#include <sstream>
#include <fstream>
#include <set>
#include <algorithm>
#include <numeric>

namespace {

constexpr char hsaIsaNamePrefix[] = "amdgcn-amd-amdhsa--";

} // namespace

namespace device {
extern const char* BlitLinearSourceCode;
extern const char* BlitImageSourceCode;

bool VirtualDevice::ActiveWait() const {
  return device_().ActiveWait();
}

}

namespace amd {

std::pair<const Isa*, const Isa*> Isa::supportedIsas() {
  constexpr amd::Isa::Feature NONE = amd::Isa::Feature::Unsupported;
  constexpr amd::Isa::Feature ANY  = amd::Isa::Feature::Any;
  constexpr amd::Isa::Feature OFF  = amd::Isa::Feature::Disabled;
  constexpr amd::Isa::Feature ON   = amd::Isa::Feature::Enabled;

  static constexpr Isa supportedIsas_[] = {

  // NOTE: Add new targets by adding rows for each permutation of the SRAMECC
  // and XNACK target feature values. If the target does not support the
  // feature then only NONE is used. If it supports the feature than include
  // rows for ANY, OFF and ON (but not NONE).
  //
  // Use the Target ID syntax. This comprises the processor name, followed by
  // the target feature settings in alphebetic order separated by ':'. If a
  // target feature is omitted it means either it is not supported, or it has
  // the ANY value. If the target feature is disabled then use a '-' suffix,
  // and if enabled use a '+' suffix.
  //
  // If the HSAIL or AMD IL compilers do not support the target, then use
  // nullptr for the ID.
  //
  // -------------- Compiler ----------    - Runtime -    ---- IP ----   -- Target -- ---------- Target Properties ----------
  //                                        Supported        Version       Features
  //                                                                                    SIMD/ SIMD  Instr Bank    LDS     Mem
  // Target ID                 HSAIL ID     ROC    PAL      Maj/Min/Stp   SRAMECC XNACK CU    Width Width Width   Size    Banks
    {"gfx700",                 "Kaveri",    true,  false,   7,  0,  0,    NONE,   NONE, 4,    16,   1,    256,    64 * Ki, 32}, // Also Spectre, Spooky, Kalindi
    {"gfx701",                 "Hawaii",    true,  false,   7,  0,  1,    NONE,   NONE, 4,    16,   1,    256,    64 * Ki, 32}, // Actually Hawaiipro
    {"gfx702",                 "gfx702",    true,  false,   7,  0,  2,    NONE,   NONE, 4,    16,   1,    256,    64 * Ki, 32}, // Actually Hawaii (can execute Hawiipro code)
    {"gfx703",                 nullptr,     false, false,   7,  0,  3,    NONE,   NONE, 4,    16,   1,    256,    64 * Ki, 32}, // Mullins
    {"gfx704",                 "Bonaire",   false, false,   7,  0,  4,    NONE,   NONE, 4,    16,   1,    256,    64 * Ki, 32},
    {"gfx705",                 "Mullins",   false, false,   7,  0,  5,    NONE,   NONE, 4,    16,   1,    256,    64 * Ki, 32}, // Actually Godavari
    {"gfx801",                 nullptr,     true,  true,    8,  0,  1,    NONE,   ANY,  4,    16,   1,    256,    64 * Ki, 32},
    {"gfx801:xnack-",          nullptr,     true,  false,   8,  0,  1,    NONE,   OFF,  4,    16,   1,    256,    64 * Ki, 32},
    {"gfx801:xnack+",          "Carrizo",   true,  true,    8,  0,  1,    NONE,   ON,   4,    16,   1,    256,    64 * Ki, 32},
    {"gfx802",                 "Tonga",     true,  true,    8,  0,  2,    NONE,   NONE, 4,    16,   1,    256,    64 * Ki, 32}, // Also Iceland
    {"gfx803",                 "Fiji",      true,  true,    8,  0,  3,    NONE,   NONE, 4,    16,   1,    256,    64 * Ki, 32}, // Also Ellesmere/Polaris10, Baffin/Polaris11, Polaris12, Polaris22/VegaM
    {"gfx805",                 nullptr,     true,  true,    8,  0,  5,    NONE,   NONE, 4,    16,   1,    256,    64 * Ki, 32}, // Tongapro
    {"gfx810",                 nullptr,     true,  true,    8,  1,  0,    NONE,   ANY,  4,    16,   1,    256,    64 * Ki, 32},
    {"gfx810:xnack-",          nullptr,     true,  false,   8,  1,  0,    NONE,   OFF,  4,    16,   1,    256,    64 * Ki, 32},
    {"gfx810:xnack+",          "Stoney",    true,  true,    8,  1,  0,    NONE,   ON,   4,    16,   1,    256,    64 * Ki, 32},
    {"gfx900",                 "gfx901",    true,  true,    9,  0,  0,    NONE,   ANY,  4,    16,   1,    256,    64 * Ki, 32}, // Also Greenland
    {"gfx900:xnack-",          "gfx900",    true,  true,    9,  0,  0,    NONE,   OFF,  4,    16,   1,    256,    64 * Ki, 32},
    {"gfx900:xnack+",          "gfx901",    true,  true,    9,  0,  0,    NONE,   ON,   4,    16,   1,    256,    64 * Ki, 32},
    {"gfx902",                 "gfx903",    true,  true,    9,  0,  2,    NONE,   ANY,  4,    16,   1,    256,    64 * Ki, 32}, // Also Raven
    {"gfx902:xnack-",          "gfx902",    true,  true,    9,  0,  2,    NONE,   OFF,  4,    16,   1,    256,    64 * Ki, 32},
    {"gfx902:xnack+",          "gfx903",    true,  true,    9,  0,  2,    NONE,   ON,   4,    16,   1,    256,    64 * Ki, 32},
    {"gfx904",                 "gfx905",    true,  true,    9,  0,  4,    NONE,   ANY,  4,    16,   1,    256,    64 * Ki, 32}, // Also Vega12
    {"gfx904:xnack-",          "gfx904",    true,  true,    9,  0,  4,    NONE,   OFF,  4,    16,   1,    256,    64 * Ki, 32},
    {"gfx904:xnack+",          "gfx905",    true,  true,    9,  0,  4,    NONE,   ON,   4,    16,   1,    256,    64 * Ki, 32},
    {"gfx906",                 "gfx907",    true,  true,    9,  0,  6,    ANY,    ANY,  4,    16,   1,    256,    64 * Ki, 32}, // Also Vega20
    {"gfx906:sramecc-",        "gfx907",    true,  true,    9,  0,  6,    OFF,    ANY,  4,    16,   1,    256,    64 * Ki, 32},
    {"gfx906:sramecc+",        nullptr,     true,  true,    9,  0,  6,    ON,     ANY,  4,    16,   1,    256,    64 * Ki, 32},
    {"gfx906:xnack-",          "gfx906",    true,  true,    9,  0,  6,    ANY,    OFF,  4,    16,   1,    256,    64 * Ki, 32},
    {"gfx906:xnack+",          "gfx907",    true,  true,    9,  0,  6,    ANY,    ON,   4,    16,   1,    256,    64 * Ki, 32},
    {"gfx906:sramecc-:xnack-", "gfx906",    true,  true,    9,  0,  6,    OFF,    OFF,  4,    16,   1,    256,    64 * Ki, 32},
    {"gfx906:sramecc-:xnack+", "gfx907",    true,  true,    9,  0,  6,    OFF,    ON,   4,    16,   1,    256,    64 * Ki, 32},
    {"gfx906:sramecc+:xnack-", nullptr,     true,  true,    9,  0,  6,    ON,     OFF,  4,    16,   1,    256,    64 * Ki, 32},
    {"gfx906:sramecc+:xnack+", nullptr,     true,  true,    9,  0,  6,    ON,     ON,   4,    16,   1,    256,    64 * Ki, 32},
    {"gfx908",                 nullptr,     true,  false,   9,  0,  8,    ANY,    ANY,  4,    16,   1,    256,    64 * Ki, 32},
    {"gfx908:sramecc-",        nullptr,     true,  false,   9,  0,  8,    OFF,    ANY,  4,    16,   1,    256,    64 * Ki, 32},
    {"gfx908:sramecc+",        nullptr,     true,  false,   9,  0,  8,    ON,     ANY,  4,    16,   1,    256,    64 * Ki, 32},
    {"gfx908:xnack-",          nullptr,     true,  false,   9,  0,  8,    ANY,    OFF,  4,    16,   1,    256,    64 * Ki, 32},
    {"gfx908:xnack+",          nullptr,     true,  false,   9,  0,  8,    ANY,    ON,   4,    16,   1,    256,    64 * Ki, 32},
    {"gfx908:sramecc-:xnack-", nullptr,     true,  false,   9,  0,  8,    OFF,    OFF,  4,    16,   1,    256,    64 * Ki, 32},
    {"gfx908:sramecc-:xnack+", nullptr,     true,  false,   9,  0,  8,    OFF,    ON,   4,    16,   1,    256,    64 * Ki, 32},
    {"gfx908:sramecc+:xnack-", nullptr,     true,  false,   9,  0,  8,    ON,     OFF,  4,    16,   1,    256,    64 * Ki, 32},
    {"gfx908:sramecc+:xnack+", nullptr,     true,  false,   9,  0,  8,    ON,     ON,   4,    16,   1,    256,    64 * Ki, 32},
    {"gfx909",                 nullptr,     false, true,    9,  0,  2,    NONE,   ANY,  4,    16,   1,    256,    64 * Ki, 32}, // Also Raven2 (can execute Raven code)
    {"gfx909:xnack-",          nullptr,     false, true,    9,  0,  2,    NONE,   OFF,  4,    16,   1,    256,    64 * Ki, 32},
    {"gfx909:xnack+",          nullptr,     false, true,    9,  0,  2,    NONE,   ON,   4,    16,   1,    256,    64 * Ki, 32},
    {"gfx90a",                 nullptr,     true,  false,   9,  0,  10,   ANY,    ANY,  4,    16,   1,    256,    64 * Ki, 32},
    {"gfx90a:sramecc-",        nullptr,     true,  false,   9,  0,  10,   OFF,    ANY,  4,    16,   1,    256,    64 * Ki, 32},
    {"gfx90a:sramecc+",        nullptr,     true,  false,   9,  0,  10,   ON,     ANY,  4,    16,   1,    256,    64 * Ki, 32},
    {"gfx90a:xnack-",          nullptr,     true,  false,   9,  0,  10,   ANY,    OFF,  4,    16,   1,    256,    64 * Ki, 32},
    {"gfx90a:xnack+",          nullptr,     true,  false,   9,  0,  10,   ANY,    ON,   4,    16,   1,    256,    64 * Ki, 32},
    {"gfx90a:sramecc-:xnack-", nullptr,     true,  false,   9,  0,  10,   OFF,    OFF,  4,    16,   1,    256,    64 * Ki, 32},
    {"gfx90a:sramecc-:xnack+", nullptr,     true,  false,   9,  0,  10,   OFF,    ON,   4,    16,   1,    256,    64 * Ki, 32},
    {"gfx90a:sramecc+:xnack-", nullptr,     true,  false,   9,  0,  10,   ON,     OFF,  4,    16,   1,    256,    64 * Ki, 32},
    {"gfx90a:sramecc+:xnack+", nullptr,     true,  false,   9,  0,  10,   ON,     ON,   4,    16,   1,    256,    64 * Ki, 32},
    {"gfx90c",                 nullptr,     true,  true,    9,  0,  12,   NONE,   ANY,  4,    16,   1,    256,    64 * Ki, 32}, // Also Renoir
    {"gfx90c:xnack-",          "gfx90c",    true,  true,    9,  0,  12,   NONE,   OFF,  4,    16,   1,    256,    64 * Ki, 32},
    {"gfx90c:xnack+",          "gfx90d",    true,  true,    9,  0,  12,   NONE,   ON,   4,    16,   1,    256,    64 * Ki, 32},
    {"gfx1010",                "gfx1010",   true,  true,    10, 1,  0,    NONE,   ANY,  2,    32,   1,    256,    64 * Ki, 32},
    {"gfx1010:xnack-",         "gfx1010",   true,  true,    10, 1,  0,    NONE,   OFF,  2,    32,   1,    256,    64 * Ki, 32},
    {"gfx1010:xnack+",         nullptr,     true,  true,    10, 1,  0,    NONE,   ON,   2,    32,   1,    256,    64 * Ki, 32},
    {"gfx1011",                "gfx1011",   true,  true,    10, 1,  1,    NONE,   ANY,  2,    32,   1,    256,    64 * Ki, 32},
    {"gfx1011:xnack-",         "gfx1011",   true,  true,    10, 1,  1,    NONE,   OFF,  2,    32,   1,    256,    64 * Ki, 32},
    {"gfx1011:xnack+",         nullptr,     true,  true,    10, 1,  1,    NONE,   ON,   2,    32,   1,    256,    64 * Ki, 32},
    {"gfx1012",                "gfx1012",   true,  true,    10, 1,  2,    NONE,   ANY,  2,    32,   1,    256,    64 * Ki, 32},
    {"gfx1012:xnack-",         "gfx1012",   true,  true,    10, 1,  2,    NONE,   OFF,  2,    32,   1,    256,    64 * Ki, 32},
    {"gfx1012:xnack+",         nullptr,     true,  true,    10, 1,  2,    NONE,   ON,   2,    32,   1,    256,    64 * Ki, 32},
    {"gfx1013",                "gfx1013",   true,  false,   10, 1,  3,    NONE,   ANY,  2,    32,   1,    256,    64 * Ki, 32},
    {"gfx1013:xnack-",         "gfx1013",   true,  false,   10, 1,  3,    NONE,   OFF,  2,    32,   1,    256,    64 * Ki, 32},
    {"gfx1013:xnack+",         nullptr,     true,  false,   10, 1,  3,    NONE,   ON,   2,    32,   1,    256,    64 * Ki, 32},
    {"gfx1030",                "gfx1030",   true,  true,    10, 3,  0,    NONE,   NONE, 2,    32,   1,    256,    64 * Ki, 32},
    {"gfx1031",                "gfx1031",   true,  true,    10, 3,  1,    NONE,   NONE, 2,    32,   1,    256,    64 * Ki, 32},
    {"gfx1032",                "gfx1032",   true,  true,    10, 3,  2,    NONE,   NONE, 2,    32,   1,    256,    64 * Ki, 32},
    {"gfx1033",                "gfx1033",   true,  false,   10, 3,  3,    NONE,   NONE, 2,    32,   1,    256,    64 * Ki, 32},
    {"gfx1034",                "gfx1034",   true,  true,    10, 3,  4,    NONE,   NONE, 2,    32,   1,    256,    64 * Ki, 32},
    {"gfx1035",                "gfx1035",   true,  true,    10, 3,  5,    NONE,   NONE, 2,    32,   1,    256,    64 * Ki, 32},
  };
  return std::make_pair(std::begin(supportedIsas_), std::end(supportedIsas_));
}

std::string Isa::processorName() const {
  std::string processor(targetId_);
  return processor.substr(0, processor.find(':'));
}

std::string Isa::isaName() const {
  return std::string(hsaIsaNamePrefix) + targetId();
}

bool Isa::isCompatible(const Isa &codeObjectIsa, const Isa &agentIsa) {
  if (codeObjectIsa.versionMajor() != agentIsa.versionMajor() ||
      codeObjectIsa.versionMinor() != agentIsa.versionMinor() ||
      codeObjectIsa.versionStepping() != agentIsa.versionStepping())
    return false;

  assert(codeObjectIsa.isSrameccSupported() == agentIsa.isSrameccSupported() &&
         agentIsa.sramecc() != Feature::Any);
  if ((codeObjectIsa.sramecc() == Feature::Enabled ||
       codeObjectIsa.sramecc() == Feature::Disabled) &&
      codeObjectIsa.sramecc() != agentIsa.sramecc())
    return false;

  assert(codeObjectIsa.isXnackSupported() == agentIsa.isXnackSupported() &&
         agentIsa.xnack() != Feature::Any);
  if ((codeObjectIsa.xnack() == Feature::Enabled || codeObjectIsa.xnack() == Feature::Disabled) &&
      codeObjectIsa.xnack() != agentIsa.xnack())
    return false;

  return true;
}

const Isa* Isa::findIsa(const char *isaName) {
  if (!isaName)
    return nullptr;
  const char* prefix = std::strstr(isaName, hsaIsaNamePrefix);
  if (prefix != isaName)
    return nullptr;
  const char *targetId = isaName + std::strlen(hsaIsaNamePrefix);
  auto supportedIsas_ = supportedIsas();
  auto isaIter = std::find_if(supportedIsas_.first, supportedIsas_.second, [&](const Isa& isa) {
    return std::strcmp(targetId, isa.targetId_) == 0;
  });
  return isaIter == supportedIsas_.second ? nullptr : isaIter;
}

const Isa* Isa::findIsa(uint32_t versionMajor, uint32_t versionMinor, uint32_t versionStepping,
                        Isa::Feature sramecc, Isa::Feature xnack) {
  auto supportedIsas_ = supportedIsas();
  auto isaIter = std::find_if(supportedIsas_.first, supportedIsas_.second, [&](const Isa& isa) {
    return versionMajor == isa.versionMajor_ && versionMinor == isa.versionMinor_ &&
        versionStepping == isa.versionStepping_ &&
        (isa.sramecc_ == amd::Isa::Feature::Unsupported || isa.sramecc_ == sramecc) &&
        (isa.xnack_ == amd::Isa::Feature::Unsupported || isa.xnack_ == xnack);
  });
  return isaIter == supportedIsas_.second ? nullptr : isaIter;
}

const Isa* Isa::begin() {
  return supportedIsas().first;
}

const Isa* Isa::end() {
  return supportedIsas().second;
}

std::vector<Device*>* Device::devices_ = nullptr;
AppProfile Device::appProfile_;

Context* Device::glb_ctx_ = nullptr;
Monitor Device::p2p_stage_ops_("P2P Staging Lock", true);
Memory* Device::p2p_stage_ = nullptr;

Monitor MemObjMap::AllocatedLock_ ROCCLR_INIT_PRIORITY(101) ("Guards MemObjMap allocation list");
std::map<uintptr_t, amd::Memory*> MemObjMap::MemObjMap_ ROCCLR_INIT_PRIORITY(101);
std::map<uintptr_t, amd::Memory*> MemObjMap::VirtualMemObjMap_ ROCCLR_INIT_PRIORITY(101);

size_t MemObjMap::size() {
  amd::ScopedLock lock(AllocatedLock_);
  return MemObjMap_.size();
}

void MemObjMap::AddMemObj(const void* k, amd::Memory* v) {
  amd::ScopedLock lock(AllocatedLock_);
  auto rval = MemObjMap_.insert({ reinterpret_cast<uintptr_t>(k), v });
  if (!rval.second) {
    DevLogPrintfError("Memobj map already has an entry for ptr: 0x%x",
                      reinterpret_cast<uintptr_t>(k));
  }
}

void MemObjMap::RemoveMemObj(const void* k) {
  amd::ScopedLock lock(AllocatedLock_);
  auto rval = MemObjMap_.erase(reinterpret_cast<uintptr_t>(k));
  if (rval != 1) {
    DevLogPrintfError("Memobj map does not have ptr: 0x%x",
                      reinterpret_cast<uintptr_t>(k));
    guarantee(false, "Memobj map does not have ptr");
  }
}

amd::Memory* MemObjMap::FindMemObj(const void* k) {
  amd::ScopedLock lock(AllocatedLock_);
  uintptr_t key = reinterpret_cast<uintptr_t>(k);
  auto it = MemObjMap_.upper_bound(key);
  if (it == MemObjMap_.begin()) {
    return nullptr;
  }

  --it;
  amd::Memory* mem = it->second;
  if (key >= it->first && key < (it->first + mem->getSize())) {
    // the k is in the range
    return mem;
  } else {
    return nullptr;
  }
}
void MemObjMap::AddVirtualMemObj(const void* k, amd::Memory* v) {
  amd::ScopedLock lock(AllocatedLock_);
  auto rval = VirtualMemObjMap_.insert({ reinterpret_cast<uintptr_t>(k), v });
  if (!rval.second) {
    DevLogPrintfError("Virtual Memobj map already has an entry for ptr: 0x%x",
                      reinterpret_cast<uintptr_t>(k));
  }
}

void MemObjMap::RemoveVirtualMemObj(const void* k) {
  amd::ScopedLock lock(AllocatedLock_);
  auto rval = VirtualMemObjMap_.erase(reinterpret_cast<uintptr_t>(k));
  if (rval != 1) {
    DevLogPrintfError("Virtual Memobj map does not have ptr: 0x%x",
                      reinterpret_cast<uintptr_t>(k));
    guarantee(false, "VirtualMemobj map does not have ptr");
  }
}

amd::Memory* MemObjMap::FindVirtualMemObj(const void* k) {
  amd::ScopedLock lock(AllocatedLock_);
  uintptr_t key = reinterpret_cast<uintptr_t>(k);
  auto it = VirtualMemObjMap_.upper_bound(key);
  if (it == VirtualMemObjMap_.begin()) {
    return nullptr;
  }

  --it;
  amd::Memory* mem = it->second;
  if (key >= it->first && key < (it->first + mem->getSize())) {
    // the k is in the range
    return mem;
  } else {
    return nullptr;
  }
}

void MemObjMap::UpdateAccess(amd::Device *peerDev) {
  if (peerDev == nullptr) {
    return;
  }

  // Provides access to all memory allocated on peerDev but
  // hsa_amd_agents_allow_access was not called because there was no peer
  amd::ScopedLock lock(AllocatedLock_);
  for (auto it : MemObjMap_) {
    const std::vector<Device*>& devices = it.second->getContext().devices();
    if (devices.size() == 1 && devices[0] == peerDev) {
      device::Memory* devMem = it.second->getDeviceMemory(*devices[0]);
      if (!devMem->getAllowedPeerAccess()) {
        peerDev->deviceAllowAccess(reinterpret_cast<void*>(it.first));
        devMem->setAllowedPeerAccess(true);
      }
    }
  }
}

void MemObjMap::Purge(amd::Device* dev) {
  assert(dev != nullptr);

  amd::ScopedLock lock(AllocatedLock_);
  for (auto it = MemObjMap_.cbegin(); it != MemObjMap_.cend(); ) {
    amd::Memory* memObj = it->second;
    unsigned int flags = memObj->getMemFlags();
    const std::vector<Device*>& devices = memObj->getContext().devices();
    if (devices.size() == 1 && devices[0] == dev && !(flags & ROCCLR_MEM_INTERNAL_MEMORY)) {
      it = MemObjMap_.erase(it);
    } else {
      ++it;
    }
  }
}

Device::BlitProgram::~BlitProgram() {
  if (program_ != nullptr) {
    program_->release();
  }
}

bool Device::BlitProgram::create(amd::Device* device, const std::string& extraKernels,
                                 const std::string& extraOptions) {
  std::vector<amd::Device*> devices;
  devices.push_back(device);
  std::string kernels(device::BlitLinearSourceCode);
  std::string image_kernels(device::BlitImageSourceCode);

  if (device->info().imageSupport_) {
    kernels += image_kernels;
  }

  if (!extraKernels.empty()) {
    kernels += extraKernels;
  }

  // Create a program with all blit kernels
  program_ = new Program(*context_, kernels.c_str(), Program::OpenCL_C);
  if (program_ == nullptr) {
    return false;
  }

  // Build all kernels
  std::string opt = "-cl-internal-kernel ";
  if (!device->settings().useLightning_) {
    opt += "-Wf,--force_disable_spir -fno-lib-no-inline -fno-sc-keep-calls ";
  }

  if (!extraOptions.empty()) {
    opt += extraOptions;
  }
  if (!GPU_DUMP_BLIT_KERNELS) {
    opt += " -fno-enable-dump";
  }
  if (CL_SUCCESS !=
      program_->build(devices, opt.c_str(), nullptr, nullptr, GPU_DUMP_BLIT_KERNELS)) {
    DevLogPrintfError("Build failed for Kernel: %s \n", kernels.c_str());
    return false;
  }
  if (!program_->load()) {
    DevLogPrintfError("Could not load the kernels: %s \n", kernels.c_str());
    return false;
  }

  return true;
}

bool Device::init() {
  assert(!Runtime::initialized() && "initialize only once");
  bool ret = false;
  devices_ = nullptr;
  appProfile_.init();


// IMPORTANT: Note that we are initialiing HSA stack first and then
// GPU stack. The order of initialization is signiicant and if changed
// amd::Device::registerDevice() must be accordingly modified.
#if defined(WITH_HSA_DEVICE)
  if ((GPU_ENABLE_PAL != 1) || flagIsDefault(GPU_ENABLE_PAL)) {
    // Return value of roc::Device::init()
    // If returned false, error initializing HSA stack.
    // If returned true, either HSA not installed or HSA stack
    //                   successfully initialized.
    ret = roc::Device::init();
    if (!ret) {
      // abort() commentted because this is the only indication
      // that KFD is not installed.
      // Ignore the failure and assume KFD is not installed.
      // abort();
      DevLogError("KFD is not installed \n");
    }
    if (!amd::IS_HIP) {
      ret |= roc::NullDevice::init();
    }
  }
#endif  // WITH_HSA_DEVICE
#if defined(WITH_PAL_DEVICE)
  if (GPU_ENABLE_PAL != 0) {
    ret |= PalDeviceLoad();
  }
#endif  // WITH_PAL_DEVICE
  return ret;
}

void Device::tearDown() {
  if (devices_ != nullptr) {
    for (uint i = 0; i < devices_->size(); ++i) {
      delete devices_->at(i);
    }
    devices_->clear();
    delete devices_;
  }
#if defined(WITH_HSA_DEVICE)
  roc::Device::tearDown();
#endif  // WITH_HSA_DEVICE
#if defined(WITH_PAL_DEVICE)
  if (GPU_ENABLE_PAL != 0) {
    PalDeviceUnload();
  }
#endif  // WITH_PAL_DEVICE
}

Device::Device()
    : settings_(nullptr),
      online_(true),
      activeWait_(false),
      blitProgram_(nullptr),
      hwDebugMgr_(nullptr),
      context_(nullptr),
      heap_buffer_(nullptr),
      arena_mem_obj_(nullptr),
      vaCacheAccess_(nullptr),
      vaCacheMap_(nullptr),
      index_(0) {
  memset(&info_, '\0', sizeof(info_));
}

Device::~Device() {
  if (vaCacheMap_) {
    CondLog(vaCacheMap_->size() != 0, "Application didn't unmap all host memory!");
    delete vaCacheMap_;
  }

  if (heap_buffer_ != nullptr) {
    delete heap_buffer_;
    heap_buffer_ = nullptr;
  }

  delete vaCacheAccess_;

  if (arena_mem_obj_ != nullptr) {
    arena_mem_obj_->release();
  }

  delete settings_;
  delete[] info_.extensions_;
}

bool Device::ValidateComgr() {
#if defined(USE_COMGR_LIBRARY)
  // Check if Lightning compiler was requested
  if (settings_->useLightning_) {
    std::call_once(amd::Comgr::initialized, amd::Comgr::LoadLib);
    // Use Lightning only if it's available
    settings_->useLightning_ = amd::Comgr::IsReady();
    return settings_->useLightning_;
  }
#endif
  return true;
}

bool Device::ValidateHsail() {
#if defined(WITH_COMPILER_LIB)
  // Check if HSAIL compiler was requested
  if (!settings_->useLightning_) {
    std::call_once(amd::Hsail::initialized, amd::Hsail::LoadLib);
    // Use Hsail only if it's available
    return amd::Hsail::IsReady();
  }
#endif
  return true;
}

bool Device::create(const Isa &isa) {
  assert(!vaCacheAccess_ && !vaCacheMap_);
  isa_ = &isa;
  vaCacheAccess_ = new amd::Monitor("VA Cache Ops Lock", true);
  if (nullptr == vaCacheAccess_) {
    return false;
  }
  vaCacheMap_ = new std::map<uintptr_t, device::Memory*>();
  if (nullptr == vaCacheMap_) {
    return false;
  }
  return true;
}

void Device::registerDevice() {
  assert(Runtime::singleThreaded() && "this is not thread-safe");

  if (devices_ == nullptr) {
    devices_ = new std::vector<Device*>;
  }

  if (info_.available_) {
    static bool defaultIsAssigned = false;
    if (!defaultIsAssigned && online_) {
      defaultIsAssigned = true;
      info_.type_ |= CL_DEVICE_TYPE_DEFAULT;
    }
  }
  if (isOnline()) {
    for (const auto& dev : devices()) {
      if (dev->isOnline()) {
        index_++;
      }
    }
  }
  devices_->push_back(this);
}

void Device::addVACache(device::Memory* memory) const {
  // Make sure system memory has direct access
  if (memory->isHostMemDirectAccess()) {
    // VA cache access must be serialised
    amd::ScopedLock lk(*vaCacheAccess_);
    void* start = memory->owner()->getHostMem();
    size_t offset;
    device::Memory* doubleMap = findMemoryFromVA(start, &offset);

    if (doubleMap == nullptr) {
      // Insert the new entry
      vaCacheMap_->insert(
          std::pair<uintptr_t, device::Memory*>(reinterpret_cast<uintptr_t>(start), memory));
    } else {
      LogError("Unexpected double map() call from the app!");
    }
  }
}

void Device::removeVACache(const device::Memory* memory) const {
  // Make sure system memory has direct access
  if (memory->isHostMemDirectAccess() && memory->owner()) {
    // VA cache access must be serialised
    amd::ScopedLock lk(*vaCacheAccess_);
    void* start = memory->owner()->getHostMem();
    vaCacheMap_->erase(reinterpret_cast<uintptr_t>(start));
  }
}

device::Memory* Device::findMemoryFromVA(const void* ptr, size_t* offset) const {
  // VA cache access must be serialised
  amd::ScopedLock lk(*vaCacheAccess_);

  uintptr_t key = reinterpret_cast<uintptr_t>(ptr);
  auto it = vaCacheMap_->upper_bound(reinterpret_cast<uintptr_t>(ptr));
  if (it == vaCacheMap_->begin()) {
    return nullptr;
  }

  --it;
  device::Memory* mem = it->second;
  if (key >= it->first && key < (it->first + mem->size())) {
    // ptr is in the range
    *offset = key - it->first;
    return mem;
  }
  return nullptr;
}

bool Device::IsTypeMatching(cl_device_type type, bool offlineDevices) {
  if (!(isOnline() || offlineDevices)) {
    return false;
  }

  return (info_.type_ & type) != 0;
}

std::vector<Device*> Device::getDevices(cl_device_type type, bool offlineDevices) {
  std::vector<Device*> result;

  if (devices_ == nullptr) {
    return result;
  }

  // Create the list of available devices
  for (const auto& it : *devices_) {
    // Check if the device type is matched
    if (it->IsTypeMatching(type, offlineDevices)) {
      result.push_back(it);
    }
  }

  return result;
}

size_t Device::numDevices(cl_device_type type, bool offlineDevices) {
  size_t result = 0;

  if (devices_ == nullptr) {
    return 0;
  }

  for (const auto& it : *devices_) {
    // Check if the device type is matched
    if (it->IsTypeMatching(type, offlineDevices)) {
      ++result;
    }
  }

  return result;
}

bool Device::getDeviceIDs(cl_device_type deviceType, uint32_t numEntries, cl_device_id* devices,
                          uint32_t* numDevices, bool offlineDevices) {
  if (numDevices != nullptr && devices == nullptr) {
    *numDevices = (uint32_t)amd::Device::numDevices(deviceType, offlineDevices);
    return (*numDevices > 0) ? true : false;
  }
  assert(devices != nullptr && "check the code above");

  std::vector<amd::Device*> ret = amd::Device::getDevices(deviceType, offlineDevices);
  if (ret.size() == 0) {
    *not_null(numDevices) = 0;
    return false;
  }

  auto it = ret.cbegin();
  uint32_t count = std::min(numEntries, (uint32_t)ret.size());

  while (count--) {
    *devices++ = as_cl(*it++);
    --numEntries;
  }
  while (numEntries--) {
    *devices++ = (cl_device_id)0;
  }

  *not_null(numDevices) = (uint32_t)ret.size();
  return true;
}

bool Device::UpdateStackSize(uint64_t stackSize) {
  uint32_t maxMemPerThread = info().localMemSizePerCU_ / info().maxThreadsPerCU_;
  if (maxMemPerThread < stackSize) {
    return false;
  }
  stack_size_ = stackSize;
  return true;
}

char* Device::getExtensionString() {
  std::stringstream extStream;
  size_t size;
  char* result = nullptr;

  // Generate the extension string
  for (uint i = 0; i < ClExtTotal; ++i) {
    if (settings().checkExtension(i)) {
      extStream << OclExtensionsString[i];
    }
  }

  size = extStream.str().size() + 1;

  // Create a single string with all extensions
  result = new char[size];
  if (result != nullptr) {
    memcpy(result, extStream.str().data(), (size - 1));
    result[size - 1] = 0;
  }

  return result;
}

}  // namespace amd

namespace device {

Settings::Settings() : value_(0) {
  assert((ClExtTotal < (8 * sizeof(extensions_))) && "Too many extensions!");
  extensions_ = 0;
  supportRA_ = true;
  customHostAllocator_ = false;
  waitCommand_ = AMD_OCL_WAIT_COMMAND;
  supportDepthsRGB_ = false;
  enableHwDebug_ = false;
  commandQueues_ = 200;  //!< Field value set to maximum number
                         //!< concurrent Virtual GPUs for default

  overrideLclSet = (!flagIsDefault(GPU_MAX_WORKGROUP_SIZE)) ? 1 : 0;
  overrideLclSet |=
      (!flagIsDefault(GPU_MAX_WORKGROUP_SIZE_2D_X) || !flagIsDefault(GPU_MAX_WORKGROUP_SIZE_2D_Y))
      ? 2
      : 0;
  overrideLclSet |=
      (!flagIsDefault(GPU_MAX_WORKGROUP_SIZE_3D_X) || !flagIsDefault(GPU_MAX_WORKGROUP_SIZE_3D_Y) ||
       !flagIsDefault(GPU_MAX_WORKGROUP_SIZE_3D_Z))
      ? 4
      : 0;

  fenceScopeAgent_ = AMD_OPT_FLUSH;
  if (amd::IS_HIP) {
    if (flagIsDefault(GPU_SINGLE_ALLOC_PERCENT)) {
      GPU_SINGLE_ALLOC_PERCENT = 100;
    }

  }
}

void Memory::saveMapInfo(const void* mapAddress, const amd::Coord3D origin,
                         const amd::Coord3D region, uint mapFlags, bool entire,
                         amd::Image* baseMip) {
  // Map/Unmap must be serialized.
  amd::ScopedLock lock(owner()->lockMemoryOps());

  WriteMapInfo info = {};
  WriteMapInfo* pInfo = &info;
  auto it = writeMapInfo_.find(mapAddress);
  if (it != writeMapInfo_.end()) {
    LogWarning("Double map of the same or overlapped region!");
    pInfo = &it->second;
  }

  if (mapFlags & (CL_MAP_WRITE | CL_MAP_WRITE_INVALIDATE_REGION)) {
    pInfo->origin_ = origin;
    pInfo->region_ = region;
    pInfo->entire_ = entire;
    pInfo->unmapWrite_ = true;
  }
  if (mapFlags & CL_MAP_READ) {
    pInfo->unmapRead_ = true;
  }
  pInfo->baseMip_ = baseMip;

  // Insert into the map if it's the first region
  if (++pInfo->count_ == 1) {
    writeMapInfo_.insert({mapAddress, info});
  }
}

ClBinary::ClBinary(const amd::Device& dev, BinaryImageFormat bifVer)
    : dev_(dev),
      binary_(nullptr),
      size_(0),
      flags_(0),
      origBinary_(nullptr),
      origSize_(0),
      encryptCode_(0),
      elfIn_(nullptr),
      elfOut_(nullptr),
      format_(bifVer) {}

ClBinary::~ClBinary() {
  release();

  delete elfIn_;
  delete elfOut_;
}

bool ClBinary::setElfTarget() {
  static const uint32_t Target = 21;
  assert(((0xFFFF8000 & Target) == 0) && "ASIC target ID >= 2^15");
  uint16_t elf_target = static_cast<uint16_t>(0x7FFF & Target);
  return elfOut()->setTarget(elf_target, amd::Elf::CAL_PLATFORM);
}

#if defined(WITH_COMPILER_LIB)
std::string ClBinary::getBIFSymbol(unsigned int symbolID) const {
  size_t nSymbols = 0;
  // Due to PRE & POST defines in bif_section_labels.hpp conflict with
  // PRE & POST struct members in sp3-si-chip-registers.h
  // unable to include bif_section_labels.hpp in device.hpp
  //! @todo: resolve conflict by renaming defines,
  // then include bif_section_labels.hpp in device.hpp &
  // use oclBIFSymbolID instead of unsigned int as a parameter
  const oclBIFSymbolID symID = static_cast<oclBIFSymbolID>(symbolID);
  switch (format_) {
    case BIF_VERSION2: {
      nSymbols = sizeof(BIF20) / sizeof(oclBIFSymbolStruct);
      const oclBIFSymbolStruct* symb = findBIFSymbolStruct(BIF20, nSymbols, symID);
      assert(symb && "BIF20 symbol with symbolID not found");
      if (symb) {
        return std::string(symb->str[bif::PRE]) + std::string(symb->str[bif::POST]);
      }
      break;
    }
    case BIF_VERSION3: {
      nSymbols = sizeof(BIF30) / sizeof(oclBIFSymbolStruct);
      const oclBIFSymbolStruct* symb = findBIFSymbolStruct(BIF30, nSymbols, symID);
      assert(symb && "BIF30 symbol with symbolID not found");
      if (symb) {
        return std::string(symb->str[bif::PRE]) + std::string(symb->str[bif::POST]);
      }
      break;
    }
    default:
      assert(0 && "unexpected BIF type");
      return "";
  }
  return "";
}
#endif

void ClBinary::init(amd::option::Options* optionsObj) {
  // option has higher priority than environment variable.
  if ((flags_ & BinarySourceMask) != BinaryRemoveSource) {
    // set to zero
    flags_ = (flags_ & (~BinarySourceMask));

    flags_ |= (optionsObj->oVariables->BinSOURCE ? BinarySaveSource : BinaryNoSaveSource);
  }

  if ((flags_ & BinaryLlvmirMask) != BinaryRemoveLlvmir) {
    // set to zero
    flags_ = (flags_ & (~BinaryLlvmirMask));

    flags_ |= (optionsObj->oVariables->BinLLVMIR ? BinarySaveLlvmir : BinaryNoSaveLlvmir);
  }

  if ((flags_ & BinaryIsaMask) != BinaryRemoveIsa) {
    // set to zero
    flags_ = (flags_ & (~BinaryIsaMask));
    flags_ |= ((optionsObj->oVariables->BinEXE) ? BinarySaveIsa : BinaryNoSaveIsa);
  }

  if ((flags_ & BinaryASMask) != BinaryRemoveAS) {
    // set to zero
    flags_ = (flags_ & (~BinaryASMask));
    flags_ |= ((optionsObj->oVariables->BinAS) ? BinarySaveAS : BinaryNoSaveAS);
  }
}

bool ClBinary::isRecompilable(std::string& llvmBinary, amd::Elf::ElfPlatform thePlatform) {
  /* It is recompilable if there is llvmir that was generated for
     the same platform (CPU or GPU) and with the same bitness.

     Note: the bitness has been checked in initClBinary(), no need
           to check it here.
   */
  if (llvmBinary.empty()) {
    DevLogError("LLVM Binary string is empty \n");
    return false;
  }

  uint16_t elf_target;
  amd::Elf::ElfPlatform platform;
  if (elfIn()->getTarget(elf_target, platform)) {
    if (platform == thePlatform) {
      return true;
    }
    if ((platform == amd::Elf::COMPLIB_PLATFORM) &&
        (((thePlatform == amd::Elf::CAL_PLATFORM) &&
          ((elf_target == (uint16_t)EM_HSAIL) ||
           (elf_target == (uint16_t)EM_HSAIL_64))) ||
         ((thePlatform == amd::Elf::CPU_PLATFORM) &&
          ((elf_target == (uint16_t)EM_386) || (elf_target == (uint16_t)EM_X86_64))))) {
      return true;
    }
  }

  DevLogPrintfError("LLVM_Binary: %s is not recompilable \n", llvmBinary.c_str());
  return false;
}

void ClBinary::release() {
  if (isBinaryAllocated() && (binary_ != nullptr)) {
    delete[] binary_;
    binary_ = nullptr;
    flags_ &= ~BinaryAllocated;
  }
}

void ClBinary::saveBIFBinary(const char* binaryIn, size_t size) {
  char* image = new char[size];
  memcpy(image, binaryIn, size);

  setBinary(image, size, true);
  return;
}

bool ClBinary::createElfBinary(bool doencrypt, Program::type_t type) {
  release();

  size_t imageSize;
  char* image;
  assert(elfOut_ && "elfOut_ should be initialized in ClBinary::data()");

  // Insert Version string that builds this binary into .comment section
  const device::Info& devInfo = dev_.info();
  std::string buildVerInfo("@(#) ");
  if (devInfo.version_ != nullptr) {
    buildVerInfo.append(devInfo.version_);
    buildVerInfo.append(".  Driver version: ");
    buildVerInfo.append(devInfo.driverVersion_);
  } else {
    // char OpenCLVersion[256];
    // size_t sz;
    // int32_t ret= clGetPlatformInfo(AMD_PLATFORM, CL_PLATFORM_VERSION, 256, OpenCLVersion, &sz);
    // if (ret == CL_SUCCESS) {
    //     buildVerInfo.append(OpenCLVersion, sz);
    // }

    // If CAL is unavailable, just hard-code the OpenCL driver version
    buildVerInfo.append("OpenCL 1.1" AMD_PLATFORM_INFO);
  }

  elfOut_->addSection(amd::Elf::COMMENT, buildVerInfo.data(), buildVerInfo.size());
  switch (type) {
    case Program::TYPE_NONE: {
      elfOut_->setType(ET_NONE);
      break;
    }
    case Program::TYPE_COMPILED: {
      elfOut_->setType(ET_REL);
      break;
    }
    case Program::TYPE_LIBRARY: {
      elfOut_->setType(ET_DYN);
      break;
    }
    case Program::TYPE_EXECUTABLE: {
      elfOut_->setType(ET_EXEC);
      break;
    }
    default:
      assert(0 && "unexpected elf type");
  }

  if (!elfOut_->dumpImage(&image, &imageSize)) {
    DevLogError("Dump Image failed \n");
    return false;
  }

  if (tempFile_) {
    std::remove(fname_.c_str());
  }

#if defined(HAVE_BLOWFISH_H)
  if (doencrypt) {
    // Increase the size by 64 to accomodate extra headers
    int outBufSize = (int)(imageSize + 64);
    char* outBuf = new char[outBufSize];
    if (outBuf == nullptr) {
      return false;
    }
    memset(outBuf, '\0', outBufSize);

    int outBytes = 0;
    bool success = amd::oclEncrypt(0, image, imageSize, outBuf, outBufSize, &outBytes);
    delete[] image;
    if (!success) {
      delete[] outBuf;
      DevLogError("Cannot succesfully OCL Encrypt Image");
      return false;
    }
    image = outBuf;
    imageSize = outBytes;
  }
#endif

  setBinary(image, imageSize, true);
  return true;
}

Program::binary_t ClBinary::data() const { return {binary_, size_}; }

Program::finfo_t ClBinary::Datafd() const { return {fdesc_, foffset_}; }

std::string ClBinary::DataURI() const { return uri_; }

bool ClBinary::setBinary(const char* theBinary, size_t theBinarySize, bool allocated,
                         amd::Os::FileDesc fdesc, size_t foffset, std::string uri) {
  release();

  size_ = theBinarySize;
  binary_ = theBinary;
  if (allocated) {
    flags_ |= BinaryAllocated;
  }

  fdesc_ = fdesc;
  foffset_ = foffset;
  uri_ = uri;

  return true;
}

void ClBinary::setFlags(int encryptCode) {
  encryptCode_ = encryptCode;
  if (encryptCode != 0) {
    flags_ =
        (flags_ &
         (~(BinarySourceMask | BinaryLlvmirMask | BinaryIsaMask | BinaryASMask)));
    flags_ |= (BinaryRemoveSource | BinaryRemoveLlvmir | BinarySaveIsa |
               BinaryRemoveAS);
  }
}

bool ClBinary::decryptElf(const char* binaryIn, size_t size, char** decryptBin, size_t* decryptSize,
                          int* encryptCode) {
  *decryptBin = nullptr;
#if defined(HAVE_BLOWFISH_H)
  int outBufSize = 0;
  if (amd::isEncryptedBIF(binaryIn, (int)size, &outBufSize)) {
    char* outBuf = new (std::nothrow) char[outBufSize];
    if (outBuf == nullptr) {
      return false;
    }

    // Decrypt
    int outDataSize = 0;
    if (!amd::oclDecrypt(binaryIn, (int)size, outBuf, outBufSize, &outDataSize)) {
      delete[] outBuf;
      DevLogError("Cannot Decrypt Image \n");
      return false;
    }

    *decryptBin = reinterpret_cast<char*>(outBuf);
    *decryptSize = outDataSize;
    *encryptCode = 1;
  }
#endif
  return true;
}

bool ClBinary::setElfIn() {
  if (elfIn_) return true;

  if (binary_ == nullptr) {
    return false;
  }
  elfIn_ = new amd::Elf(ELFCLASSNONE, binary_, size_, nullptr, amd::Elf::ELF_C_READ);
  if ((elfIn_ == nullptr) || !elfIn_->isSuccessful()) {
    delete elfIn_;
    elfIn_ = nullptr;
    LogError("Creating input ELF object failed");
    return false;
  }

  return true;
}

void ClBinary::resetElfIn() {
  delete elfIn_;
  elfIn_ = nullptr;
}

bool ClBinary::setElfOut(unsigned char eclass,
                         const char* outFile, bool tempFile) {
  elfOut_ = new amd::Elf(eclass, nullptr, 0, outFile, amd::Elf::ELF_C_WRITE);
  if ((elfOut_ == nullptr) || !elfOut_->isSuccessful()) {
    delete elfOut_;
    elfOut_ = nullptr;
    LogError("Creating output ELF object failed");
    return false;
  }

  fname_ = outFile;
  tempFile_ = tempFile;

  return setElfTarget();
}

void ClBinary::resetElfOut() {
  delete elfOut_;
  elfOut_ = nullptr;
}

bool ClBinary::loadLlvmBinary(std::string& llvmBinary,
                              amd::Elf::ElfSections& elfSectionType) const {
  // Check if current binary already has LLVMIR
  char* section = nullptr;
  size_t sz = 0;
  const amd::Elf::ElfSections SectionTypes[] = {amd::Elf::LLVMIR, amd::Elf::SPIR,
                                                      amd::Elf::SPIRV};

  for (int i = 0; i < 3; ++i) {
    if (elfIn_->getSection(SectionTypes[i], &section, &sz) && section && sz > 0) {
      llvmBinary.append(section, sz);
      elfSectionType = SectionTypes[i];
      return true;
    }
  }

  DevLogPrintfError("Cannot Load LLVM Binary: %s \n", llvmBinary.c_str());
  return false;
}

bool ClBinary::loadCompileOptions(std::string& compileOptions) const {
  char* options = nullptr;
  size_t sz;
  compileOptions.clear();
#if defined(WITH_COMPILER_LIB)
  if (elfIn_->getSymbol(amd::Elf::COMMENT, getBIFSymbol(symOpenclCompilerOptions).c_str(),
                        &options, &sz)) {
    if (sz > 0) {
      compileOptions.append(options, sz);
    }
    return true;
  }
#endif
  return false;
}

bool ClBinary::loadLinkOptions(std::string& linkOptions) const {
  char* options = nullptr;
  size_t sz;
  linkOptions.clear();
#if defined(WITH_COMPILER_LIB)
  if (elfIn_->getSymbol(amd::Elf::COMMENT, getBIFSymbol(symOpenclLinkerOptions).c_str(),
                        &options, &sz)) {
    if (sz > 0) {
      linkOptions.append(options, sz);
    }
    return true;
  }
#endif
  return false;
}

void ClBinary::storeCompileOptions(const std::string& compileOptions) {
#if defined(WITH_COMPILER_LIB)
  elfOut()->addSymbol(amd::Elf::COMMENT, getBIFSymbol(symOpenclCompilerOptions).c_str(),
                      compileOptions.c_str(), compileOptions.length());
#endif
}

void ClBinary::storeLinkOptions(const std::string& linkOptions) {
#if defined(WITH_COMPILER_LIB)
  elfOut()->addSymbol(amd::Elf::COMMENT, getBIFSymbol(symOpenclLinkerOptions).c_str(),
                      linkOptions.c_str(), linkOptions.length());
#endif
}

bool ClBinary::isSPIR() const {
  char* section = nullptr;
  size_t sz = 0;
  if (elfIn_->getSection(amd::Elf::LLVMIR, &section, &sz) && section && sz > 0) return false;

  if (elfIn_->getSection(amd::Elf::SPIR, &section, &sz) && section && sz > 0) return true;

  return false;
}

bool ClBinary::isSPIRV() const {
  char* section = nullptr;
  size_t sz = 0;

  if (elfIn_->getSection(amd::Elf::SPIRV, &section, &sz) && section && sz > 0) {
    return true;
  }
  return false;
}

}  // namespace device
