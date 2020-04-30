# ROCclr - Radeon Open Compute Common Language Runtime 
ROCclr is a virtual device interface that compute runtimes interact with to different backends such as ROCr or PAL
This abstraction allows runtimes to work on Windows as well as on Linux without much effort.

To build:

 Prerequisites

  Install mesa-common-dev
 
  Either build or install comgr & clang
 
 
 git clone https://github.com/ROCm-Developer-Tools/ROCclr.git
 
 export VDI_DIR="$(readlink -f ROCclr)"
 
 git clone -b master-next https://github.com/RadeonOpenCompute/ROCm-OpenCL-Runtime.git
 
 export OPENCL_DIR="$(readlink -f ROCm-OpenCL-Runtime)"
 
 cd ../ROCclr
 
 mkdir -p build; cd build
 
 cmake -DOPENCL_DIR="$OPENCL_DIR" -DCMAKE_INSTALL_PREFIX=/opt/rocm/vdi ..
 
 make
 
 
For release build, add "-DCMAKE_BUILD_TYPE=Release" to the cmake command line. This make 10% difference in some benchmark test.

(Optional) Build the HIP runtime

 git clone  -b master-next https://github.com/ROCm-Developer-Tools/HIP.git
 
 export HIP_DIR="$(readlink -f hip)"
 
 cd "$HIP_DIR"
 
 mkdir -p build; cd build
 
 cmake -DHIP_COMPILER=clang -DHIP_PLATFORM=vdi -DVDI_DIR="$VDI_DIR" -DLIBVDI_STATIC_DIR="$VDI_DIR/build" ..
 
 make
