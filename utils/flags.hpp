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

#ifndef FLAGS_HPP_
#define FLAGS_HPP_


#define RUNTIME_FLAGS(debug,release,release_on_stg)                           \
                                                                              \
release(int, AMD_LOG_LEVEL, 0,                                                \
        "The default log level")                                              \
release(uint, AMD_LOG_MASK, 0X7FFFFFFF,                                       \
        "The mask to enable specific kinds of logs")                          \
debug(uint, DEBUG_GPU_FLAGS, 0,                                               \
        "The debug options for GPU device")                                   \
release(uint, GPU_MAX_COMMAND_QUEUES, 300,                                    \
        "The maximum number of concurrent Virtual GPUs")                      \
release(size_t, CQ_THREAD_STACK_SIZE, 256*Ki, /* @todo: that much! */         \
        "The default command queue thread stack size")                        \
release(int, GPU_MAX_WORKGROUP_SIZE, 0,                                       \
        "Maximum number of workitems in a workgroup for GPU, 0 -use default") \
release(int, GPU_MAX_WORKGROUP_SIZE_2D_X, 0,                                  \
        "Maximum number of workitems in a 2D workgroup for GPU, x component, 0 -use default") \
release(int, GPU_MAX_WORKGROUP_SIZE_2D_Y, 0,                                  \
        "Maximum number of workitems in a 2D workgroup for GPU, y component, 0 -use default") \
release(int, GPU_MAX_WORKGROUP_SIZE_3D_X, 0,                                  \
        "Maximum number of workitems in a 3D workgroup for GPU, x component, 0 -use default") \
release(int, GPU_MAX_WORKGROUP_SIZE_3D_Y, 0,                                  \
        "Maximum number of workitems in a 3D workgroup for GPU, y component, 0 -use default") \
release(int, GPU_MAX_WORKGROUP_SIZE_3D_Z, 0,                                  \
        "Maximum number of workitems in a 3D workgroup for GPU, z component, 0 -use default") \
debug(bool, CPU_MEMORY_GUARD_PAGES, false,                                    \
        "Use guard pages for CPU memory")                                     \
debug(size_t, CPU_MEMORY_GUARD_PAGE_SIZE, 64,                                 \
        "Size in KB of CPU memory guard page")                                \
debug(size_t, CPU_MEMORY_ALIGNMENT_SIZE, 256,                                 \
        "Size in bytes for the default alignment for guarded memory on CPU")  \
debug(size_t, PARAMETERS_MIN_ALIGNMENT, 16,                                   \
        "Minimum alignment required for the abstract parameters stack")       \
debug(size_t, MEMOBJ_BASE_ADDR_ALIGN, 4*Ki,                                   \
        "Alignment of the base address of any allocate memory object")        \
release(uint, ROC_HMM_FLAGS, 0,                                               \
        "ROCm HMM configuration flags")                                       \
release(cstring, GPU_DEVICE_ORDINAL, "",                                      \
        "Select the device ordinal (comma seperated list of available devices)") \
release(bool, REMOTE_ALLOC, false,                                            \
        "Use remote memory for the global heap allocation")                   \
release(uint, GPU_MAX_HEAP_SIZE, 100,                                         \
        "Set maximum size of the GPU heap to % of board memory")              \
release(uint, GPU_STAGING_BUFFER_SIZE, 1024,                                  \
        "Size of the GPU staging buffer in KiB")                              \
release(bool, GPU_DUMP_BLIT_KERNELS, false,                                   \
        "Dump the kernels for blit manager")                                  \
release(uint, GPU_BLIT_ENGINE_TYPE, 0x0,                                      \
        "Blit engine type: 0 - Default, 1 - Host, 2 - CAL, 3 - Kernel")       \
release(bool, GPU_FLUSH_ON_EXECUTION, false,                                  \
        "Submit commands to HW on every operation. 0 - Disable, 1 - Enable")  \
release(bool, GPU_USE_SYNC_OBJECTS, true,                                     \
        "If enabled, use sync objects instead of polling")                    \
release(bool, CL_KHR_FP64, true,                                              \
        "Enable/Disable support for double precision")                        \
release(cstring, AMD_OCL_BUILD_OPTIONS, 0,                                    \
        "Set clBuildProgram() and clCompileProgram()'s options (override)")   \
release(cstring, AMD_OCL_BUILD_OPTIONS_APPEND, 0,                             \
        "Append clBuildProgram() and clCompileProgram()'s options")           \
release(cstring, AMD_OCL_LINK_OPTIONS, 0,                                     \
        "Set clLinkProgram()'s options (override)")                           \
release(cstring, AMD_OCL_LINK_OPTIONS_APPEND, 0,                              \
        "Append clLinkProgram()'s options")                                   \
release(cstring, AMD_OCL_SC_LIB, 0,                                           \
        "Set shader compiler shared library name or path")                    \
debug(cstring, AMD_OCL_SUBST_OBJFILE, 0,                                      \
        "Specify binary substitution config file for OpenCL")                 \
debug(bool, AMD_OCL_ENABLE_MESSAGE_BOX, false,                                \
        "Enable the error dialog on Windows")                                 \
release(size_t, GPU_PINNED_XFER_SIZE, 32,                                     \
        "The pinned buffer size for pinning in read/write transfers")         \
release(size_t, GPU_PINNED_MIN_XFER_SIZE, 1024,                               \
        "The minimal buffer size for pinned read/write transfers in KBytes")  \
release(size_t, GPU_RESOURCE_CACHE_SIZE, 64,                                  \
        "The resource cache size in MB")                                      \
release(size_t, GPU_MAX_SUBALLOC_SIZE, 4096,                                  \
        "The maximum size accepted for suballocaitons in KB")                 \
release(bool, GPU_FORCE_64BIT_PTR, 0,                                         \
        "Forces 64 bit pointers on GPU")                                      \
release(bool, GPU_FORCE_OCL20_32BIT, 0,                                       \
        "Forces 32 bit apps to take CLANG\HSAIL path")                        \
release(bool, GPU_RAW_TIMESTAMP, 0,                                           \
        "Reports GPU raw timestamps in GPU timeline")                         \
release(size_t, GPU_NUM_MEM_DEPENDENCY, 256,                                  \
        "Number of memory objects for dependency tracking")                   \
release(size_t, GPU_XFER_BUFFER_SIZE, 0,                                      \
        "Transfer buffer size for image copy optimization in KB")             \
release(bool, GPU_IMAGE_DMA, true,                                            \
        "Enable DRM DMA for image transfers")                                 \
release(uint, GPU_SINGLE_ALLOC_PERCENT, 85,                                   \
        "Maximum size of a single allocation as percentage of total")         \
release(uint, GPU_NUM_COMPUTE_RINGS, 2,                                       \
        "GPU number of compute rings. 0 - disabled, 1 , 2,.. - the number of compute rings") \
release(int, GPU_SELECT_COMPUTE_RINGS_ID, -1,                                 \
        "GPU select the compute rings ID -1 - disabled, 0 , 1,.. - the forced compute rings ID for submission") \
release(uint, GPU_WORKLOAD_SPLIT, 22,                                         \
        "Workload split size")                                                \
release(bool, GPU_USE_SINGLE_SCRATCH, false,                                  \
        "Use single scratch buffer per device instead of per HW ring")        \
release(bool, AMD_OCL_WAIT_COMMAND, false,                                    \
        "1 = Enable a wait for every submitted command")                      \
release(uint, GPU_PRINT_CHILD_KERNEL, 0,                                      \
        "Prints the specified number of the child kernels")                   \
release(bool, GPU_USE_DEVICE_QUEUE, false,                                    \
        "Use a dedicated device queue for the actual submissions")            \
release(bool, GPU_ENABLE_LARGE_ALLOCATION, true,                              \
        "Enable >4GB single allocations")                                     \
release(bool, AMD_THREAD_TRACE_ENABLE, true,                                  \
        "Enable thread trace extension")                                      \
release(uint, OPENCL_VERSION, (IS_BRAHMA ? 120 : 200),                        \
        "Force GPU opencl verison")                                           \
release(bool, HSA_LOCAL_MEMORY_ENABLE, true,                                  \
        "Enable HSA device local memory usage")                               \
release(uint, HSA_KERNARG_POOL_SIZE, 512 * 1024,                              \
        "Kernarg pool size")                                                  \
release(bool, HSA_ENABLE_COARSE_GRAIN_SVM, true,                              \
        "Enable device memory for coarse grain SVM allocations")              \
release(bool, GPU_IFH_MODE, false,                                            \
        "1 = Enable GPU IFH (infinitely fast hardware) mode. Any other value keeps setting disabled.") \
release(bool, GPU_MIPMAP, true,                                               \
        "Enables GPU mipmap extension")                                       \
release(uint, GPU_ENABLE_PAL, 2,                                              \
        "Enables PAL backend. 0 - ROC, 1 - PAL, 2 - ROC or PAL")              \
release(bool, DISABLE_DEFERRED_ALLOC, false,                                  \
        "Disables deferred memory allocation on device")                      \
release(int, AMD_GPU_FORCE_SINGLE_FP_DENORM, -1,                              \
        "Force denorm for single precision: -1 - don't force, 0 - disable, 1 - enable") \
release(uint, OCL_SET_SVM_SIZE, 4*16384,                                      \
        "set SVM space size for discrete GPU")                                \
debug(uint, OCL_SYSMEM_REQUIREMENT, 2,                                        \
        "Use flag to change the minimum requirement of system memory not to downgrade")        \
debug(bool, GPU_ENABLE_HW_DEBUG, false,                                       \
        "Enable HW DEBUG for GPU")                                            \
release(uint, GPU_WAVES_PER_SIMD, 0,                                          \
        "Force the number of waves per SIMD (1-10)")                          \
release(bool, GPU_WAVE_LIMIT_ENABLE, false,                                   \
        "1 = Enable adaptive wave limiter")                                   \
release(bool, OCL_STUB_PROGRAMS, false,                                       \
        "1 = Enables OCL programs stubing")                                   \
release(bool, GPU_ANALYZE_HANG, false,                                        \
        "1 = Enables GPU hang analysis")                                      \
release(uint, GPU_MAX_REMOTE_MEM_SIZE, 2,                                     \
        "Maximum size (in Ki) that allows device memory substitution with system") \
release(bool, GPU_ADD_HBCC_SIZE, false,                                        \
        "Add HBCC size to the reported device memory")                        \
release_on_stg(uint, GPU_WAVE_LIMIT_CU_PER_SH, 0,                             \
        "Assume the number of CU per SH for wave limiter")                    \
release_on_stg(uint, GPU_WAVE_LIMIT_MAX_WAVE, 10,                             \
        "Set maximum waves per SIMD to try for wave limiter")                 \
release_on_stg(uint, GPU_WAVE_LIMIT_RUN, 20,                                  \
        "Set running factor for wave limiter")                                \
release_on_stg(cstring, GPU_WAVE_LIMIT_DUMP, "",                              \
        "File path prefix for dumping wave limiter output")                   \
release_on_stg(cstring, GPU_WAVE_LIMIT_TRACE, "",                             \
        "File path prefix for tracing wave limiter")                          \
release(bool, OCL_CODE_CACHE_ENABLE, false,                                   \
        "1 = Enable compiler code cache")                                     \
release(bool, OCL_CODE_CACHE_RESET, false,                                    \
        "1 =  Reset the compiler code cache storage")                         \
release_on_stg(bool, PAL_DISABLE_SDMA, false,                                 \
        "1 = Disable SDMA for PAL")                                           \
release(uint, PAL_RGP_DISP_COUNT, 50,                                         \
        "The number of dispatches for RGP capture with SQTT")                 \
release(uint, PAL_MALL_POLICY, 0,                                             \
        "Controls the behaviour of allocations with respect to the MALL"      \
        "0 = MALL policy is decided by KMD"                                   \
        "1 = Allocations are never put through the MALL"                      \
        "2 = Allocations will always be put through the MALL")                \
release(bool, GPU_ENABLE_WAVE32_MODE, true,                                   \
        "Enables Wave32 compilation in HW if available")                      \
release(bool, GPU_ENABLE_LC, true,                                            \
        "Enables LC path")                                                    \
release(bool, GPU_ENABLE_HW_P2P, false,                                       \
        "Enables HW P2P path")                                                \
release(bool, GPU_ENABLE_COOP_GROUPS, true,                                   \
         "Enables cooperative group launch")                                  \
release(uint, GPU_MAX_COMMAND_BUFFERS, 8,                                     \
         "The maximum number of command buffers allocated per queue")         \
release(uint, GPU_MAX_HW_QUEUES, 4,                                           \
         "The maximum number of HW queues allocated per device")              \
release(bool, GPU_IMAGE_BUFFER_WAR, true,                                     \
        "Enables image buffer workaround")                                    \
release(cstring, HIP_VISIBLE_DEVICES, "",                                     \
        "Only devices whose index is present in the sequence are visible to HIP")  \
release(cstring, CUDA_VISIBLE_DEVICES, "",                                    \
        "Only devices whose index is present in the sequence are visible to CUDA") \
release(bool, GPU_ENABLE_WGP_MODE, true,                                      \
        "Enables WGP Mode in HW if available")                                \
release(bool, GPU_DUMP_CODE_OBJECT, false,                                    \
        "Enable dump code object")                                            \
release(uint, GPU_MAX_USWC_ALLOC_SIZE, 2048,                                  \
        "Set a limit in Mb on the maximum USWC allocation size"               \
        "-1 = No limit")                                                      \
release(uint, AMD_SERIALIZE_KERNEL, 0,                                        \
        "Serialize kernel enqueue, 0x1 = Wait for completion before enqueue"  \
        "0x2 = Wait for completion after enqueue 0x3 = both")                 \
release(uint, AMD_SERIALIZE_COPY, 0,                                          \
        "Serialize copies, 0x1 = Wait for completion before enqueue"          \
        "0x2 = Wait for completion after enqueue 0x3 = both")                 \
release(bool, PAL_ALWAYS_RESIDENT, false,                                     \
        "Force memory resources to become resident at allocation time")       \
release(uint, HIP_HOST_COHERENT, 0,                                           \
        "Coherent memory in hipHostMalloc, 0x1 = memory is coherent with host"\
        "0x0 = memory is not coherent between host and GPU")                  \
release(uint, AMD_OPT_FLUSH, 1,                                               \
        "Kernel flush option , 0x0 = Use system-scope fence operations."      \
        "0x1 = Use device-scope fence operations when possible.")             \
release(bool, AMD_DIRECT_DISPATCH, false,                                     \
        "Enable direct kernel dispatch.")                                     \
release(uint, HIP_HIDDEN_FREE_MEM, 0,                                         \
        "Reserve free mem reporting in Mb"                                    \
        "0 = Disable")                                                        \
release(size_t, GPU_FORCE_BLIT_COPY_SIZE, 0,                                  \
        "Size in KB of the threshold below which to force blit instead for sdma") \
release(uint, ROC_ACTIVE_WAIT_TIMEOUT, 10,                                    \
        "Forces active wait of GPU interrup for the timeout(us)")             \
release(bool, ROC_ENABLE_LARGE_BAR, true,                                     \
        "Enable Large Bar if supported by the device")                        \
release(bool, ROC_CPU_WAIT_FOR_SIGNAL, true,                                  \
        "Enable CPU wait for dependent HSA signals.")                         \
release(bool, ROC_SYSTEM_SCOPE_SIGNAL, true,                                  \
        "Enable system scope for signals (uses interrupts).")                 \
release(bool, ROC_SKIP_COPY_SYNC, false,                                      \
        "Skips copy syncs if runtime can predict the same engine.")           \
release(bool, ROC_ENABLE_PRE_VEGA, false,                                     \
        "Enable support of pre-vega ASICs in ROCm path")                      \
release(bool, HIP_FORCE_QUEUE_PROFILING, false,                               \
        "Force command queue profiling by default")                           \
release(bool, HIP_MEM_POOL_SUPPORT, false,                                    \
        "Enables memory pool support in HIP")                                 \
release(uint, PAL_FORCE_ASIC_REVISION, 0,                                     \
        "Force a specific asic revision for all devices")                     \
release(bool, PAL_EMBED_KERNEL_MD, false,                                     \
        "Enables writing kernel metadata into command buffers.")              \
release(cstring, ROC_GLOBAL_CU_MASK, "",                                      \
        "Sets a global CU mask (entered as hex value) for all queues,"        \
        "Each active bit represents using one CU (e.g., 0xf enables only 4 CUs)") \
release(cstring, AMD_LOG_LEVEL_FILE, "",                                      \
        "Set output file for AMD_LOG_LEVEL, Default is stderr")               \
release(size_t, PAL_PREPINNED_MEMORY_SIZE, 64,                                \
        "Size in KBytes of prepinned memory")                                 \
release(bool, AMD_CPU_AFFINITY, false,                                        \
        "Reset CPU affinity of any runtime threads")                          \
release(bool, ROC_USE_FGS_KERNARG, true,                                      \
        "Use fine grain kernel args segment for supported asics")             \
release(uint, ROC_P2P_SDMA_SIZE, 1024,                                        \
        "The minimum size in KB for P2P transfer with SDMA")                  \
release(uint, ROC_AQL_QUEUE_SIZE, 4096,                                       \
        "AQL queue size in AQL packets")                                      \
release(bool, ROC_SKIP_KERNEL_ARG_COPY, false,                                \
        "If true, then runtime can skip kernel arg copy")                     \
release(bool, GPU_STREAMOPS_CP_WAIT, false,                                   \
        "Force the stream wait memory operation to wait on CP.")              \
release(bool, ROC_EVENT_NO_FLUSH, false,                                      \
        "Use NOP AQL packet for event records with no explicit flags.")

namespace amd {

extern bool IS_HIP;
extern std::atomic_bool IS_PROFILER_ON;

extern bool IS_LEGACY;

//! \addtogroup Utils
//  @{

struct Flag {
  enum Type {
    Tinvalid = 0,
    Tbool,    //!< A boolean type flag (true, false).
    Tint,     //!< An integer type flag (signed).
    Tuint,    //!< An integer type flag (unsigned).
    Tsize_t,  //!< A size_t type flag.
    Tcstring  //!< A string type flag.
  };

#define DEFINE_FLAG_NAME(type, name, value, help) k##name,
  enum Name {
    RUNTIME_FLAGS(DEFINE_FLAG_NAME, DEFINE_FLAG_NAME, DEFINE_FLAG_NAME)
    numFlags_
  };
#undef DEFINE_FLAG_NAME

#define CAN_SET(type, name, v, h)    static const bool cannotSet##name = false;
#define CANNOT_SET(type, name, v, h) static const bool cannotSet##name = true;

#ifdef DEBUG
  RUNTIME_FLAGS(CAN_SET, CAN_SET, CAN_SET)
#else // !DEBUG
  RUNTIME_FLAGS(CANNOT_SET, CAN_SET, CANNOT_SET)
#endif // !DEBUG

#undef CAN_SET
#undef CANNOT_SET

 private:
  static Flag flags_[];

 public:
  static char* envstr_;
  const char* name_;
  const void* value_;
  Type type_;
  bool isDefault_;

 public:
  static bool init();

  static void tearDown();

  bool setValue(const char* value);

  static bool isDefault(Name name) { return flags_[name].isDefault_; }
};

#define flagIsDefault(name) \
  (amd::Flag::cannotSet##name || amd::Flag::isDefault(amd::Flag::k##name))

#define setIfNotDefault(var, opt, other) \
  if (!flagIsDefault(opt)) \
    var = (opt);\
  else \
    var = (other);

//  @}

} // namespace amd

#ifdef _WIN32
# define EXPORT_FLAG extern "C" __declspec(dllexport)
#else // !_WIN32
# define EXPORT_FLAG extern "C"
#endif // !_WIN32

#define DECLARE_RELEASE_FLAG(type, name, value, help) EXPORT_FLAG type name;
#ifdef DEBUG
# define DECLARE_DEBUG_FLAG(type, name, value, help) EXPORT_FLAG type name;
#else // !DEBUG
# define DECLARE_DEBUG_FLAG(type, name, value, help) const type name = value;
#endif // !DEBUG

RUNTIME_FLAGS(DECLARE_DEBUG_FLAG, DECLARE_RELEASE_FLAG, DECLARE_DEBUG_FLAG);

#undef DECLARE_DEBUG_FLAG
#undef DECLARE_RELEASE_FLAG

#endif /*FLAGS_HPP_*/
