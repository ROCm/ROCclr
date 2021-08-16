/* Copyright (c) 2010 - 2021 Advanced Micro Devices, Inc.

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

#pragma once

#ifndef WITHOUT_HSA_BACKEND

/*! \addtogroup HSA OCL Stub Implementation
 *  @{
 */

//! HSA OCL STUB Implementation
namespace roc {

//! Device settings
class Settings : public device::Settings {
 public:
  enum Hmm : uint32_t {
    EnableSystemMemory    = 0x01, //!< Forces system memory preference by default
    EnableMallocPrefetch  = 0x02, //!< Skips default prefetch after allocation
    EnableSvmTracking     = 0x04, //!< Enables SW SVM tracking
    EnableDebugSvm        = 0x08  //!< Extra debug flag (reserved for runtime developers)
  };

  union {
    struct {
      uint doublePrecision_ : 1;        //!< Enables double precision support
      uint enableLocalMemory_ : 1;      //!< Enable GPUVM memory
      uint enableCoarseGrainSVM_ : 1;   //!< Enable device memory for coarse grain SVM allocations
      uint enableNCMode_ : 1;           //!< Enable Non Coherent mode for system memory
      uint imageDMA_ : 1;               //!< Enable direct image DMA transfers
      uint stagedXferRead_ : 1;         //!< Uses a staged buffer read
      uint stagedXferWrite_ : 1;        //!< Uses a staged buffer write
      uint imageBufferWar_ : 1;         //!< Image buffer workaround for Gfx10
      uint cpu_wait_for_signal_ : 1;    //!< Wait for HSA signal on CPU
      uint system_scope_signal_ : 1;    //!< HSA signal is visibile to the entire system
      uint skip_copy_sync_ : 1;         //!< Ignore explicit HSA signal waits for copy functionality
      uint reserved_ : 21;
    };
    uint value_;
  };

  //! Default max workgroup size for 1D
  int maxWorkGroupSize_;

  //! Preferred workgroup size
  uint preferredWorkGroupSize_;

  //! Default max workgroup sizes for 2D
  int maxWorkGroupSize2DX_;
  int maxWorkGroupSize2DY_;

  //! Default max workgroup sizes for 3D
  int maxWorkGroupSize3DX_;
  int maxWorkGroupSize3DY_;
  int maxWorkGroupSize3DZ_;

  uint kernargPoolSize_;
  uint numDeviceEvents_;      //!< The number of device events
  uint numWaitEvents_;        //!< The number of wait events for device enqueue

  size_t xferBufSize_;        //!< Transfer buffer size for image copy optimization
  size_t stagedXferSize_;     //!< Staged buffer size
  size_t pinnedXferSize_;     //!< Pinned buffer size for transfer
  size_t pinnedMinXferSize_;  //!< Minimal buffer size for pinned transfer

  size_t sdmaCopyThreshold_;  //!< Use SDMA to copy above this size

  uint32_t  hmmFlags_;        //!< HMM functionality control flags

  //! Default constructor
  Settings();

  //! Creates settings
  bool create(bool fullProfile, uint32_t gfxipMajor, uint32_t gfxipMinor, bool enableXNACK,
              bool coop_groups = false);

 private:
  //! Disable copy constructor
  Settings(const Settings&);

  //! Disable assignment
  Settings& operator=(const Settings&);

  //! Overrides current settings based on registry/environment
  void override();
};

/*@}*/} // namespace roc

#endif /*WITHOUT_HSA_BACKEND*/
