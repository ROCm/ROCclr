/* Copyright (c) 2021 - 2021 Advanced Micro Devices, Inc.

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

#include "rocsignal.hpp"

namespace roc {

Signal::~Signal() {
  hsa_signal_destroy(signal_);
}

bool Signal::Init(const amd::Device& dev, uint64_t init, device::Signal::WaitState ws) {
  hsa_status_t status = hsa_signal_create(init, 0, nullptr, &signal_);
  if (status != HSA_STATUS_SUCCESS) {
    return false;
  }

  ws_ = ws;

  return true;
}

uint64_t Signal::Wait(uint64_t value, device::Signal::Condition c, uint64_t timeout) {
  return hsa_signal_wait_scacquire(
    signal_,
    static_cast<hsa_signal_condition_t>(c),
    value,
    timeout,
    static_cast<hsa_wait_state_t>(ws_));
}

void Signal::Reset(uint64_t value) {
  hsa_signal_store_screlease(signal_, value);
}

};