# Copyright (c) 2020 - 2021 Advanced Micro Devices, Inc. All rights reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

find_package(amd_comgr REQUIRED CONFIG
  PATHS
    /opt/rocm/
    ${ROCM_INSTALL_PATH}
  PATH_SUFFIXES
    cmake/amd_comgr
    lib/cmake/amd_comgr)

target_compile_definitions(rocclr PUBLIC WITH_LIGHTNING_COMPILER USE_COMGR_LIBRARY)
if(BUILD_SHARED_LIBS)
  target_compile_definitions(rocclr PUBLIC COMGR_DYN_DLL)
endif()
target_link_libraries(rocclr PUBLIC amd_comgr)

if(${CMAKE_PROJECT_NAME} STREQUAL "hip")
  add_definitions(-DHIP_MAJOR_VERSION=${HIP_VERSION_MAJOR})
  add_definitions(-DHIP_MINOR_VERSION=${HIP_VERSION_MINOR})
endif()
