/* Copyright (c) 2009 - 2021 Advanced Micro Devices, Inc.

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

#ifndef WITHOUT_HSA_BACKEND

#include "top.hpp"
#include "device/device.hpp"
#include "device/appprofile.hpp"
#include "device/rocm/rocappprofile.hpp"

#include <algorithm>

amd::AppProfile* rocCreateAppProfile() {
  amd::AppProfile* appProfile = new roc::AppProfile;

  if ((appProfile == nullptr) || !appProfile->init()) {
    DevLogPrintfError("App Profile init failed, appProfile: 0x%x \n",
                      appProfile);
    return nullptr;
  }

  return appProfile;
}

namespace roc {

bool AppProfile::ParseApplicationProfile() {
  std::string appName("Explorer");

  std::transform(appName.begin(), appName.end(), appName.begin(), ::tolower);
  std::transform(appFileName_.begin(), appFileName_.end(), appFileName_.begin(), ::tolower);

  if (appFileName_.compare(appName) == 0) {
    gpuvmHighAddr_ = false;
    profileOverridesAllSettings_ = true;
  }

  return true;
}
}

#endif
