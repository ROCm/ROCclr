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

#include "platform/runtime.hpp"
#include "os/os.hpp"
#include "thread/thread.hpp"
#include "device/device.hpp"
#include "utils/flags.hpp"
#include "utils/options.hpp"
#include "platform/context.hpp"
#include "platform/agent.hpp"

#include "amdocl/cl_gl_amd.hpp"

#ifdef _WIN32
#include <d3d10_1.h>
#include <dxgi.h>
#include "CL/cl_d3d10.h"
#endif  //_WIN32

#if defined(_MSC_VER)  // both Win32 and Win64
#include <intrin.h>
#endif

#include <atomic>
#include <cstdlib>
#include <iostream>

namespace amd {

volatile bool Runtime::initialized_ = false;

bool Runtime::init() {
  if (initialized_) {
    return true;
  }

  // Enter a very basic critical region. We want to prevent 2 threads
  // from concurrently executing the init() routines. We can't use a
  // Monitor since the system is not yet initialized.

  static std::atomic_flag lock = ATOMIC_FLAG_INIT;
  struct CriticalRegion {
    std::atomic_flag& lock_;
    CriticalRegion(std::atomic_flag& lock) : lock_(lock) {
      while (lock.test_and_set(std::memory_order_acquire)) {
        Os::yield();
      }
    }
    ~CriticalRegion() { lock_.clear(std::memory_order_release); }
  } region(lock);

  if (initialized_) {
    return true;
  }

  if (!Flag::init() || !option::init() || !Device::init()
      // Agent initializes last
      || !Agent::init()) {
    ClPrint(LOG_ERROR, LOG_INIT, "Runtime initialization failed");
    return false;
  }

  initialized_ = true;
  ClTrace(LOG_DEBUG, LOG_INIT);
  return true;
}

void Runtime::tearDown() {
  if (!initialized_) {
    return;
  }
  ClTrace(LOG_DEBUG, LOG_INIT);

  Agent::tearDown();
  Device::tearDown();
  option::teardown();
  Flag::tearDown();
  if (outFile != stderr && outFile != nullptr) {
    fclose(outFile);
  }
  initialized_ = false;
}

class RuntimeTearDown : public amd::HeapObject {
public:
  RuntimeTearDown() {}
  ~RuntimeTearDown() { /*Runtime::tearDown();*/ }
} runtime_tear_down;

uint ReferenceCountedObject::retain() {
  return referenceCount_.fetch_add(1, std::memory_order_relaxed) + 1;
}

uint ReferenceCountedObject::release() {
  uint newCount = referenceCount_.fetch_sub(1, std::memory_order_relaxed) - 1;
  if (newCount == 0) {
    if (terminate()) {
      delete this;
    }
  }
  return newCount;
}

}  // namespace amd
