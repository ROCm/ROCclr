# ROCclr - Radeon Open Compute Common Language Runtime 
ROCclr is a virtual device interface that compute runtimes interact with to different backends such as ROCr or PAL
This abstraction allows runtimes to work on Windows as well as on Linux without much effort.

## Repository branches
The repository maintains several branches. The branches that are of importance are:

- master: This is the default branch.

## Building

###Prerequisites

- Install mesa-common-dev
- Either build or install comgr & clang

### Getting the source code

```
git clone https://github.com/ROCm-Developer-Tools/ROCclr.git
git clone -b master-next https://github.com/RadeonOpenCompute/ROCm-OpenCL-Runtime.git
```

### Set the environment variables

```
export ROCclr_DIR="$(readlink -f ROCclr)"
export OPENCL_DIR="$(readlink -f ROCm-OpenCL-Runtime)"
```

###Build ROCclr
Here is command to build ROCclr:
```
cd "$ROCclr_DIR"
mkdir -p build; cd build
cmake -DOPENCL_DIR="$OPENCL_DIR" -DCMAKE_INSTALL_PREFIX=/opt/rocm/vdi ..
make
```

### Optional steps to build HIP runtime
Enter the directory where git cloned the ROCClr and OpenCL. Run the following commands: [^1]
```
git clone  -b master-next https://github.com/ROCm-Developer-Tools/HIP.git
export HIP_DIR="$(readlink -f HIP)"
cd "$HIP_DIR"
mkdir -p build; cd build
cmake -DHIP_COMPILER=clang -DHIP_PLATFORM=vdi -DVDI_DIR="$ROCclr_DIR" -DLIBVDI_STATIC_DIR="$ROCclr_DIR/build" ..
make
```
###Release build
For release build, add "-DCMAKE_BUILD_TYPE=Release" to the cmake command line. This make 10% difference in some benchmark test.

[^1]: In future, the cmake command to build HIP runtime will be simplied like this: ```cmake -DHIP_COMPILER=clang -DHIP_PLATFORM=rocclr -DCMAKE_PREFIX_PATH="$ROCclr_DIR/build" ..```
