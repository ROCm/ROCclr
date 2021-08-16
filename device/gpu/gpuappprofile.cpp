/* Copyright (c) 2014 - 2021 Advanced Micro Devices, Inc.

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

#include "top.hpp"
#include "utils/debug.hpp"
#include "device/appprofile.hpp"
#include "device/gpu/gpuappprofile.hpp"

namespace gpu {

AppProfile::AppProfile()
    : amd::AppProfile(), enableHighPerformanceState_(true), reportAsOCL12Device_(false) {
  propertyDataMap_.insert({"HighPerfState", PropertyData(DataType_Boolean, &enableHighPerformanceState_)});
  propertyDataMap_.insert({"OCL12Device", PropertyData(DataType_Boolean, &reportAsOCL12Device_)});
  propertyDataMap_.insert({"SclkThreshold", PropertyData(DataType_String, &sclkThreshold_)});
  propertyDataMap_.insert({"DownHysteresis", PropertyData(DataType_String, &downHysteresis_)});
  propertyDataMap_.insert({"UpHysteresis", PropertyData(DataType_String, &upHysteresis_)});
  propertyDataMap_.insert({"PowerLimit", PropertyData(DataType_String, &powerLimit_)});
  propertyDataMap_.insert({"MclkThreshold", PropertyData(DataType_String, &mclkThreshold_)});
  propertyDataMap_.insert({"MclkUpHyst", PropertyData(DataType_String, &mclkUpHyst_)});
  propertyDataMap_.insert({"MclkDownHyst", PropertyData(DataType_String, &mclkDownHyst_)});
}
}
