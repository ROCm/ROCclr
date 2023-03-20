/* Copyright (c) 2009 - 2023 Advanced Micro Devices, Inc.

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

#ifdef _WIN32

#include "top.hpp"

#include "platform/interop_d3d11.hpp"
#include "platform/command.hpp"

#include <cstring>
#include <utility>

namespace amd {

size_t D3D11Object::getResourceByteSize() {
  size_t bytes = 1;

  //! @todo [odintsov]: take into consideration the mip level?!

  switch (objDesc_.objDim_) {
    case D3D11_RESOURCE_DIMENSION_BUFFER:
      bytes = objDesc_.objSize_.ByteWidth;
      break;

    case D3D11_RESOURCE_DIMENSION_TEXTURE3D:
      bytes = objDesc_.objSize_.Depth;

    case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
      bytes *= objDesc_.objSize_.Height;

    case D3D11_RESOURCE_DIMENSION_TEXTURE1D:
      bytes *= objDesc_.objSize_.Width * getElementBytes();
      break;

    default:
      LogError("getResourceByteSize: unknown type of D3D11 resource");
      bytes = 0;
      break;
  }
  return bytes;
}

cl_uint D3D11Object::getMiscFlag() {
  if ((objDesc_.dxgiFormat_ == DXGI_FORMAT_NV12) ||
      (objDesc_.dxgiFormat_ == DXGI_FORMAT_P010)) {
    return 1;
  }
  else if (objDesc_.dxgiFormat_ == DXGI_FORMAT_YUY2) {
    return 3;
  }
  return 0;
}

int D3D11Object::initD3D11Object(const Context& amdContext, ID3D11Resource* pRes, UINT subres,
                                 D3D11Object& obj, INT plane) {
  ID3D11Device* pDev;
  HRESULT hr;
  ScopedLock sl(resLock_);

  // Check if this ressource has already been used for interop
  for (const auto& it : resources_) {
    if (it.first == (void*)pRes && it.second.first == subres &&
        it.second.second == plane) {
      return CL_INVALID_D3D11_RESOURCE_KHR;
    }
  }

  (obj.pD3D11Res_ = pRes)->GetDevice(&pDev);

  if (!pDev) {
    return CL_INVALID_D3D11_DEVICE_KHR;
  }

  D3D11_QUERY_DESC desc = {D3D11_QUERY_EVENT, 0};
  pDev->CreateQuery(&desc, &obj.pQuery_);

#define SET_SHARED_FLAGS()                                                                         \
  {                                                                                                \
    obj.pD3D11ResOrig_ = obj.pD3D11Res_;                                                           \
    /* @todo - Check device type and select right usage for resource */                            \
    /* For now get only DPU path, CPU path for buffers */                                          \
    /* will not worl on DEFAUL resources */                                                        \
    /*desc.Usage = D3D11_USAGE_STAGING;*/                                                          \
    desc.Usage = D3D11_USAGE_DEFAULT;                                                              \
    desc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;                                                   \
    desc.CPUAccessFlags = 0;                                                                       \
  }

#define STORE_SHARED_FLAGS_BUFFER(restype)                                                         \
  {                                                                                                \
    if (S_OK == hr && obj.pD3D11Res_) {                                                            \
      obj.objDesc_.objFlags_.d3d11Usage_ = desc.Usage;                                             \
      obj.objDesc_.objFlags_.bindFlags_ = desc.BindFlags;                                          \
      obj.objDesc_.objFlags_.miscFlags_ = desc.MiscFlags;                                          \
      obj.objDesc_.objFlags_.cpuAccessFlags_ = desc.CPUAccessFlags;                                \
      obj.objDesc_.objFlags_.structureByteStride_ = desc.StructureByteStride;                      \
    } else {                                                                                       \
      LogError("\nCannot create shared " #restype "\n");                                           \
      return CL_INVALID_D3D11_RESOURCE_KHR;                                                        \
    }                                                                                              \
  }

#define STORE_SHARED_FLAGS(restype)                                                                \
  {                                                                                                \
    if (S_OK == hr && obj.pD3D11Res_) {                                                            \
      obj.objDesc_.objFlags_.d3d11Usage_ = desc.Usage;                                             \
      obj.objDesc_.objFlags_.bindFlags_ = desc.BindFlags;                                          \
      obj.objDesc_.objFlags_.miscFlags_ = desc.MiscFlags;                                          \
      obj.objDesc_.objFlags_.cpuAccessFlags_ = desc.CPUAccessFlags;                                \
    } else {                                                                                       \
      LogError("\nCannot create shared " #restype "\n");                                           \
      return CL_INVALID_D3D11_RESOURCE_KHR;                                                        \
    }                                                                                              \
  }

#define SET_BINDING()                                                                              \
  {                                                                                                \
    switch (desc.Format) {                                                                         \
      case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:                                                       \
      case DXGI_FORMAT_D32_FLOAT:                                                                  \
      case DXGI_FORMAT_D24_UNORM_S8_UINT:                                                          \
      case DXGI_FORMAT_D16_UNORM:                                                                  \
        desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;                                                 \
        break;                                                                                     \
      default:                                                                                     \
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;                    \
        break;                                                                                     \
    }                                                                                              \
  }

  pRes->GetType(&obj.objDesc_.objDim_);

  // Init defaults
  obj.objDesc_.objSize_.Height = 1;
  obj.objDesc_.objSize_.Depth = 1;
  obj.objDesc_.mipLevels_ = 1;
  obj.objDesc_.arraySize_ = 1;
  obj.objDesc_.dxgiFormat_ = DXGI_FORMAT_UNKNOWN;
  obj.objDesc_.dxgiSampleDesc_ = dxgiSampleDescDefault;

  switch (obj.objDesc_.objDim_) {
    case D3D11_RESOURCE_DIMENSION_BUFFER:  // = 1,
    {
      D3D11_BUFFER_DESC desc;
      (reinterpret_cast<ID3D11Buffer*>(pRes))->GetDesc(&desc);
      obj.objDesc_.objSize_.ByteWidth = desc.ByteWidth;
      obj.objDesc_.objFlags_.d3d11Usage_ = desc.Usage;
      obj.objDesc_.objFlags_.bindFlags_ = desc.BindFlags;
      obj.objDesc_.objFlags_.cpuAccessFlags_ = desc.CPUAccessFlags;
      obj.objDesc_.objFlags_.miscFlags_ = desc.MiscFlags;
      obj.objDesc_.objFlags_.structureByteStride_ = desc.StructureByteStride;
      // Handle D3D11Buffer without shared handle - create
      //  a duplicate with shared handle to provide for CAL
      if (!(obj.objDesc_.objFlags_.miscFlags_ & D3D11_RESOURCE_MISC_SHARED)) {
        SET_SHARED_FLAGS();
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
        hr = pDev->CreateBuffer(&desc, nullptr, (ID3D11Buffer**)&obj.pD3D11Res_);
        STORE_SHARED_FLAGS_BUFFER(ID3D11Buffer);
      }
    } break;

    case D3D11_RESOURCE_DIMENSION_TEXTURE1D:  // = 2,
    {
      D3D11_TEXTURE1D_DESC desc;
      (reinterpret_cast<ID3D11Texture1D*>(pRes))->GetDesc(&desc);

      if (subres) {
        // Calculate correct size of the subresource
        UINT miplevel = subres;
        if (desc.ArraySize > 1) {
          miplevel = subres % desc.ArraySize;
        }
        if (miplevel >= desc.MipLevels) {
          LogWarning("\nMiplevel >= number of miplevels\n");
        }
        if (subres >= desc.MipLevels * desc.ArraySize) {
          return CL_INVALID_VALUE;
        }
        desc.Width >>= miplevel;
        if (!desc.Width) {
          desc.Width = 1;
        }
      }
      obj.objDesc_.objSize_.Width = desc.Width;
      obj.objDesc_.mipLevels_ = desc.MipLevels;
      obj.objDesc_.arraySize_ = desc.ArraySize;
      obj.objDesc_.dxgiFormat_ = desc.Format;
      obj.objDesc_.objFlags_.d3d11Usage_ = desc.Usage;
      obj.objDesc_.objFlags_.bindFlags_ = desc.BindFlags;
      obj.objDesc_.objFlags_.cpuAccessFlags_ = desc.CPUAccessFlags;
      obj.objDesc_.objFlags_.miscFlags_ = desc.MiscFlags;
      // Handle D3D11Texture1D without shared handle - create
      //  a duplicate with shared handle and provide it for CAL
      // Workaround for subresource > 0 in shared resource
      if (subres) obj.objDesc_.objFlags_.miscFlags_ &= ~(D3D11_RESOURCE_MISC_SHARED);
      if (!(obj.objDesc_.objFlags_.miscFlags_ & D3D11_RESOURCE_MISC_SHARED)) {
        SET_SHARED_FLAGS();
        SET_BINDING();
        obj.objDesc_.mipLevels_ = desc.MipLevels = 1;
        obj.objDesc_.arraySize_ = desc.ArraySize = 1;
        hr = pDev->CreateTexture1D(&desc, nullptr, (ID3D11Texture1D**)&obj.pD3D11Res_);
        STORE_SHARED_FLAGS(ID3D11Texture1D);
      }
    } break;

    case D3D11_RESOURCE_DIMENSION_TEXTURE2D:  // = 3,
    {
      D3D11_TEXTURE2D_DESC desc;
      (reinterpret_cast<ID3D11Texture2D*>(pRes))->GetDesc(&desc);

      if (subres) {
        // Calculate correct size of the subresource
        UINT miplevel = subres;
        if (desc.ArraySize > 1) {
          miplevel = subres % desc.MipLevels;
        }
        if (miplevel >= desc.MipLevels) {
          LogWarning("\nMiplevel >= number of miplevels\n");
        }
        if (subres >= desc.MipLevels * desc.ArraySize) {
          return CL_INVALID_VALUE;
        }
        desc.Width >>= miplevel;
        if (!desc.Width) {
          desc.Width = 1;
        }
        desc.Height >>= miplevel;
        if (!desc.Height) {
          desc.Height = 1;
        }
      }
      obj.objDesc_.objSize_.Width = desc.Width;
      obj.objDesc_.objSize_.Height = desc.Height;
      obj.objDesc_.mipLevels_ = desc.MipLevels;
      obj.objDesc_.arraySize_ = desc.ArraySize;
      obj.objDesc_.dxgiFormat_ = desc.Format;
      obj.objDesc_.dxgiSampleDesc_ = desc.SampleDesc;
      obj.objDesc_.objFlags_.d3d11Usage_ = desc.Usage;
      obj.objDesc_.objFlags_.bindFlags_ = desc.BindFlags;
      obj.objDesc_.objFlags_.cpuAccessFlags_ = desc.CPUAccessFlags;
      obj.objDesc_.objFlags_.miscFlags_ = desc.MiscFlags;

      // Handle D3D11Texture2D without shared handle - create
      //  a duplicate with shared handle and provide it for CAL
      // Workaround for subresource > 0 in shared resource
      if (subres) obj.objDesc_.objFlags_.miscFlags_ &= ~(D3D11_RESOURCE_MISC_SHARED);
      if (!(obj.objDesc_.objFlags_.miscFlags_ & D3D11_RESOURCE_MISC_SHARED)) {
        SET_SHARED_FLAGS();
        SET_BINDING();
        obj.objDesc_.mipLevels_ = desc.MipLevels = 1;
        obj.objDesc_.arraySize_ = desc.ArraySize = 1;
        hr = pDev->CreateTexture2D(&desc, nullptr, (ID3D11Texture2D**)&obj.pD3D11Res_);
        STORE_SHARED_FLAGS(ID3D11Texture2D);
      }

      if ((desc.Format == DXGI_FORMAT_NV12) || (desc.Format == DXGI_FORMAT_P010)) {
        if (plane == -1) {
          obj.objDesc_.objSize_.Height += obj.objDesc_.objSize_.Height / 2;
        }
        if (plane == 1) {
          obj.objDesc_.objSize_.Width /= 2;
          obj.objDesc_.objSize_.Height /= 2;
        }
      }
      // RGBA8 covers 2 pixels, thus divide width by 2
      if (desc.Format == DXGI_FORMAT_YUY2) {
        obj.objDesc_.objSize_.Width /= 2;
      }
    } break;

    case D3D11_RESOURCE_DIMENSION_TEXTURE3D:  // = 4
    {
      D3D11_TEXTURE3D_DESC desc;
      (reinterpret_cast<ID3D11Texture3D*>(pRes))->GetDesc(&desc);

      if (subres) {
        // Calculate correct size of the subresource
        UINT miplevel = subres;
        if (miplevel >= desc.MipLevels) {
          LogWarning("\nMiplevel >= number of miplevels\n");
        }
        if (subres >= desc.MipLevels) {
          return CL_INVALID_VALUE;
        }
        desc.Width >>= miplevel;
        if (!desc.Width) {
          desc.Width = 1;
        }
        desc.Height >>= miplevel;
        if (!desc.Height) {
          desc.Height = 1;
        }
        desc.Depth >>= miplevel;
        if (!desc.Depth) {
          desc.Depth = 1;
        }
      }
      obj.objDesc_.objSize_.Width = desc.Width;
      obj.objDesc_.objSize_.Height = desc.Height;
      obj.objDesc_.objSize_.Depth = desc.Depth;
      obj.objDesc_.mipLevels_ = desc.MipLevels;
      obj.objDesc_.dxgiFormat_ = desc.Format;
      obj.objDesc_.objFlags_.d3d11Usage_ = desc.Usage;
      obj.objDesc_.objFlags_.bindFlags_ = desc.BindFlags;
      obj.objDesc_.objFlags_.cpuAccessFlags_ = desc.CPUAccessFlags;
      obj.objDesc_.objFlags_.miscFlags_ = desc.MiscFlags;
      // Handle D3D11Texture3D without shared handle - create
      //  a duplicate with shared handle and provide it for CAL
      // Workaround for subresource > 0 in shared resource
      if (obj.objDesc_.mipLevels_ > 1)
        obj.objDesc_.objFlags_.miscFlags_ &= ~(D3D11_RESOURCE_MISC_SHARED);
      if (!(obj.objDesc_.objFlags_.miscFlags_ & D3D11_RESOURCE_MISC_SHARED)) {
        SET_SHARED_FLAGS();
        SET_BINDING();
        obj.objDesc_.mipLevels_ = desc.MipLevels = 1;
        hr = pDev->CreateTexture3D(&desc, nullptr, (ID3D11Texture3D**)&obj.pD3D11Res_);
        STORE_SHARED_FLAGS(ID3D11Texture3D);
      }
    } break;

    default:
      LogError("unknown type of D3D11 resource");
      return CL_INVALID_D3D11_RESOURCE_KHR;
  }
  obj.subRes_ = subres;
  obj.plane_ = plane;
  pDev->Release();
  // Check for CL format compatibilty
  if (obj.objDesc_.objDim_ != D3D11_RESOURCE_DIMENSION_BUFFER) {
    cl_image_format clFmt = obj.getCLFormatFromDXGI(obj.objDesc_.dxgiFormat_, plane);
    amd::Image::Format imageFormat(clFmt);
    if (!imageFormat.isSupported(amdContext)) {
      return CL_INVALID_IMAGE_FORMAT_DESCRIPTOR;
    }
  }
  resources_.push_back({pRes, {subres, plane}});
  return CL_SUCCESS;
}

bool D3D11Object::copyOrigToShared() {
  // Don't copy if there is no orig
  if (nullptr == getD3D11ResOrig()) return true;

  ID3D11Device* d3dDev;
  pD3D11Res_->GetDevice(&d3dDev);
  if (!d3dDev) {
    LogError("\nCannot get D3D11 device from D3D11 resource\n");
    return false;
  }
  ID3D11DeviceContext* pImmediateContext = nullptr;
  d3dDev->GetImmediateContext(&pImmediateContext);
  if (!pImmediateContext) {
    LogError("\nCannot get D3D11 device context");
    return false;
  }
  assert(pD3D11ResOrig_ != nullptr);
  // Any usage source can be read by GPU
  pImmediateContext->CopySubresourceRegion(pD3D11Res_, 0, 0, 0, 0, pD3D11ResOrig_, subRes_, nullptr);

  // Flush D3D queues and make sure D3D stuff is finished
  {
    ScopedLock sl(resLock_);  // protect from multiple
    pImmediateContext->Flush();
    pImmediateContext->End(pQuery_);
    BOOL data = FALSE;
    while (S_OK != pImmediateContext->GetData(pQuery_, &data, sizeof(BOOL), 0)) {
    }
  }

  pImmediateContext->Release();
  d3dDev->Release();
  return true;
}

bool D3D11Object::copySharedToOrig() {
  // Don't copy if there is no orig
  if (nullptr == getD3D11ResOrig()) return true;

  ID3D11Device* d3dDev;
  pD3D11Res_->GetDevice(&d3dDev);
  if (!d3dDev) {
    LogError("\nCannot get D3D11 device from D3D11 resource\n");
    return false;
  }
  ID3D11DeviceContext* pImmediateContext = nullptr;
  d3dDev->GetImmediateContext(&pImmediateContext);
  if (!pImmediateContext) {
    LogError("\nCannot get D3D11 device context");
    return false;
  }
  assert(pD3D11ResOrig_);
  pImmediateContext->CopySubresourceRegion(pD3D11ResOrig_, subRes_, 0, 0, 0, pD3D11Res_, 0, nullptr);
  pImmediateContext->Release();

  d3dDev->Release();
  return true;
}

std::vector<std::pair<void*, std::pair<UINT, UINT>>> D3D11Object::resources_;
Monitor D3D11Object::resLock_;

//
// Class BufferD3D11 implementation
//
void BufferD3D11::initDeviceMemory() {
  deviceMemories_ =
      reinterpret_cast<DeviceMemory*>(reinterpret_cast<char*>(this) + sizeof(BufferD3D11));
  memset(deviceMemories_, 0, context_().devices().size() * sizeof(DeviceMemory));
}

//
// Class Image1DD3D11 implementation
//
void Image1DD3D11::initDeviceMemory() {
  deviceMemories_ =
      reinterpret_cast<DeviceMemory*>(reinterpret_cast<char*>(this) + sizeof(Image1DD3D11));
  memset(deviceMemories_, 0, context_().devices().size() * sizeof(DeviceMemory));
}

//
// Class Image2DD3D11 implementation
//

void Image2DD3D11::initDeviceMemory() {
  deviceMemories_ =
      reinterpret_cast<DeviceMemory*>(reinterpret_cast<char*>(this) + sizeof(Image2DD3D11));
  memset(deviceMemories_, 0, context_().devices().size() * sizeof(DeviceMemory));
}

//
// Class Image3DD3D11 implementation
//
void Image3DD3D11::initDeviceMemory() {
  deviceMemories_ =
      reinterpret_cast<DeviceMemory*>(reinterpret_cast<char*>(this) + sizeof(Image3DD3D11));
  memset(deviceMemories_, 0, context_().devices().size() * sizeof(DeviceMemory));
}



//
// Class D3D11Object implementation
//
size_t D3D11Object::getElementBytes(DXGI_FORMAT dxgiFmt, cl_uint plane) {
  size_t bytesPerPixel;

  switch (dxgiFmt) {
    case DXGI_FORMAT_R32G32B32A32_TYPELESS:
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
    case DXGI_FORMAT_R32G32B32A32_UINT:
    case DXGI_FORMAT_R32G32B32A32_SINT:
      bytesPerPixel = 16;
      break;

    case DXGI_FORMAT_R32G32B32_TYPELESS:
    case DXGI_FORMAT_R32G32B32_FLOAT:
    case DXGI_FORMAT_R32G32B32_UINT:
    case DXGI_FORMAT_R32G32B32_SINT:
      bytesPerPixel = 12;
      break;

    case DXGI_FORMAT_R16G16B16A16_TYPELESS:
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
    case DXGI_FORMAT_R16G16B16A16_UNORM:
    case DXGI_FORMAT_R16G16B16A16_UINT:
    case DXGI_FORMAT_R16G16B16A16_SNORM:
    case DXGI_FORMAT_R16G16B16A16_SINT:
    case DXGI_FORMAT_R32G32_TYPELESS:
    case DXGI_FORMAT_R32G32_FLOAT:
    case DXGI_FORMAT_R32G32_UINT:
    case DXGI_FORMAT_R32G32_SINT:
    case DXGI_FORMAT_R32G8X24_TYPELESS:
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
    case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
    case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
      bytesPerPixel = 8;
      break;

    case DXGI_FORMAT_R10G10B10A2_TYPELESS:
    case DXGI_FORMAT_R10G10B10A2_UNORM:
    case DXGI_FORMAT_R10G10B10A2_UINT:
    case DXGI_FORMAT_R11G11B10_FLOAT:
    case DXGI_FORMAT_R8G8B8A8_TYPELESS:
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
    case DXGI_FORMAT_R8G8B8A8_UINT:
    case DXGI_FORMAT_R8G8B8A8_SNORM:
    case DXGI_FORMAT_R8G8B8A8_SINT:
    case DXGI_FORMAT_R16G16_TYPELESS:
    case DXGI_FORMAT_R16G16_FLOAT:
    case DXGI_FORMAT_R16G16_UNORM:
    case DXGI_FORMAT_R16G16_UINT:
    case DXGI_FORMAT_R16G16_SNORM:
    case DXGI_FORMAT_R16G16_SINT:
    case DXGI_FORMAT_R32_TYPELESS:
    case DXGI_FORMAT_D32_FLOAT:
    case DXGI_FORMAT_R32_FLOAT:
    case DXGI_FORMAT_R32_UINT:
    case DXGI_FORMAT_R32_SINT:
    case DXGI_FORMAT_R24G8_TYPELESS:
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
    case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
    case DXGI_FORMAT_X24_TYPELESS_G8_UINT:

    case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
    case DXGI_FORMAT_R8G8_B8G8_UNORM:
    case DXGI_FORMAT_G8R8_G8B8_UNORM:

    case DXGI_FORMAT_B8G8R8A8_UNORM:
    case DXGI_FORMAT_B8G8R8X8_UNORM:

    case DXGI_FORMAT_YUY2:
      bytesPerPixel = 4;
      break;

    case DXGI_FORMAT_R8G8_TYPELESS:
    case DXGI_FORMAT_R8G8_UNORM:
    case DXGI_FORMAT_R8G8_UINT:
    case DXGI_FORMAT_R8G8_SNORM:
    case DXGI_FORMAT_R8G8_SINT:
    case DXGI_FORMAT_R16_TYPELESS:
    case DXGI_FORMAT_R16_FLOAT:
    case DXGI_FORMAT_D16_UNORM:
    case DXGI_FORMAT_R16_UNORM:
    case DXGI_FORMAT_R16_UINT:
    case DXGI_FORMAT_R16_SNORM:
    case DXGI_FORMAT_R16_SINT:

    case DXGI_FORMAT_B5G6R5_UNORM:
    case DXGI_FORMAT_B5G5R5A1_UNORM:
      bytesPerPixel = 2;
      break;

    case DXGI_FORMAT_R8_TYPELESS:
    case DXGI_FORMAT_R8_UNORM:
    case DXGI_FORMAT_R8_UINT:
    case DXGI_FORMAT_R8_SNORM:
    case DXGI_FORMAT_R8_SINT:
    case DXGI_FORMAT_A8_UNORM:
    case DXGI_FORMAT_R1_UNORM:
      bytesPerPixel = 1;
      break;


    case DXGI_FORMAT_BC1_TYPELESS:
    case DXGI_FORMAT_BC1_UNORM:
    case DXGI_FORMAT_BC1_UNORM_SRGB:
    case DXGI_FORMAT_BC2_TYPELESS:
    case DXGI_FORMAT_BC2_UNORM:
    case DXGI_FORMAT_BC2_UNORM_SRGB:
    case DXGI_FORMAT_BC3_TYPELESS:
    case DXGI_FORMAT_BC3_UNORM:
    case DXGI_FORMAT_BC3_UNORM_SRGB:
    case DXGI_FORMAT_BC4_TYPELESS:
    case DXGI_FORMAT_BC4_UNORM:
    case DXGI_FORMAT_BC4_SNORM:
    case DXGI_FORMAT_BC5_TYPELESS:
    case DXGI_FORMAT_BC5_UNORM:
    case DXGI_FORMAT_BC5_SNORM:
      // Less than 1 byte per pixel - needs special consideration
      bytesPerPixel = 0;
      break;
    case DXGI_FORMAT_NV12:
      bytesPerPixel = 1;
      if (plane == 1) {
        bytesPerPixel = 2;
      }
      break;
    case DXGI_FORMAT_P010:
        bytesPerPixel = 2;
        if (plane == 1) {
            bytesPerPixel = 4;
        }
        break;
    default:
      bytesPerPixel = 0;
      _ASSERT(FALSE);
      break;
  }
  return bytesPerPixel;
}

cl_image_format D3D11Object::getCLFormatFromDXGI(DXGI_FORMAT dxgiFmt, cl_uint plane) {
  cl_image_format fmt;

  //! @todo [odintsov]: add real fmt conversion from DXGI to CL
  fmt.image_channel_order = 0;      // CL_RGBA;
  fmt.image_channel_data_type = 0;  // CL_UNSIGNED_INT8;

  switch (dxgiFmt) {
    case DXGI_FORMAT_R32G32B32A32_TYPELESS:
      fmt.image_channel_order = CL_RGBA;
      break;

    case DXGI_FORMAT_R32G32B32A32_FLOAT:
      fmt.image_channel_order = CL_RGBA;
      fmt.image_channel_data_type = CL_FLOAT;
      break;

    case DXGI_FORMAT_R32G32B32A32_UINT:
      fmt.image_channel_order = CL_RGBA;
      fmt.image_channel_data_type = CL_UNSIGNED_INT32;
      break;

    case DXGI_FORMAT_R32G32B32A32_SINT:
      fmt.image_channel_order = CL_RGBA;
      fmt.image_channel_data_type = CL_SIGNED_INT32;
      break;

    case DXGI_FORMAT_R32G32B32_TYPELESS:
      fmt.image_channel_order = CL_RGB;
      break;

    case DXGI_FORMAT_R32G32B32_FLOAT:
      fmt.image_channel_order = CL_RGB;
      fmt.image_channel_data_type = CL_FLOAT;
      break;

    case DXGI_FORMAT_R32G32B32_UINT:
      fmt.image_channel_order = CL_RGB;
      fmt.image_channel_data_type = CL_UNSIGNED_INT32;
      break;

    case DXGI_FORMAT_R32G32B32_SINT:
      fmt.image_channel_order = CL_RGB;
      fmt.image_channel_data_type = CL_SIGNED_INT32;
      break;

    case DXGI_FORMAT_R16G16B16A16_TYPELESS:
      fmt.image_channel_order = CL_RGBA;
      break;

    case DXGI_FORMAT_R16G16B16A16_FLOAT:
      fmt.image_channel_order = CL_RGBA;
      fmt.image_channel_data_type = CL_HALF_FLOAT;
      break;

    case DXGI_FORMAT_R16G16B16A16_UNORM:
      fmt.image_channel_order = CL_RGBA;
      fmt.image_channel_data_type = CL_UNORM_INT16;
      break;

    case DXGI_FORMAT_R16G16B16A16_UINT:
      fmt.image_channel_order = CL_RGBA;
      fmt.image_channel_data_type = CL_UNSIGNED_INT16;
      break;

    case DXGI_FORMAT_R16G16B16A16_SNORM:
      fmt.image_channel_order = CL_RGBA;
      fmt.image_channel_data_type = CL_SNORM_INT16;
      break;

    case DXGI_FORMAT_R16G16B16A16_SINT:
      fmt.image_channel_order = CL_RGBA;
      fmt.image_channel_data_type = CL_SIGNED_INT16;
      break;

    case DXGI_FORMAT_R32G32_TYPELESS:
      fmt.image_channel_order = CL_RG;
      break;

    case DXGI_FORMAT_R32G32_FLOAT:
      fmt.image_channel_order = CL_RG;
      fmt.image_channel_data_type = CL_FLOAT;
      break;

    case DXGI_FORMAT_R32G32_UINT:
      fmt.image_channel_order = CL_RG;
      fmt.image_channel_data_type = CL_UNSIGNED_INT32;
      break;

    case DXGI_FORMAT_R32G32_SINT:
      fmt.image_channel_order = CL_RG;
      fmt.image_channel_data_type = CL_SIGNED_INT32;
      break;

    case DXGI_FORMAT_R32G8X24_TYPELESS:
      break;

    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
      break;

    case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
      break;

    case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
      break;

    case DXGI_FORMAT_R10G10B10A2_TYPELESS:
      fmt.image_channel_order = CL_RGBA;
      break;

    case DXGI_FORMAT_R10G10B10A2_UNORM:
      fmt.image_channel_order = CL_RGBA;
      fmt.image_channel_data_type = CL_UNORM_INT_101010;
      break;

    case DXGI_FORMAT_R10G10B10A2_UINT:
      fmt.image_channel_order = CL_RGBA;
      break;

    case DXGI_FORMAT_R11G11B10_FLOAT:
      fmt.image_channel_order = CL_RGB;
      break;

    case DXGI_FORMAT_R8G8B8A8_TYPELESS:
      fmt.image_channel_order = CL_RGBA;
      break;

    case DXGI_FORMAT_R8G8B8A8_UNORM:
      fmt.image_channel_order = CL_RGBA;
      fmt.image_channel_data_type = CL_UNORM_INT8;
      break;

    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
      fmt.image_channel_order = CL_RGBA;
      fmt.image_channel_data_type = CL_UNORM_INT8;
      break;

    case DXGI_FORMAT_R8G8B8A8_UINT:
    case DXGI_FORMAT_YUY2:
      fmt.image_channel_order = CL_RGBA;
      fmt.image_channel_data_type = CL_UNSIGNED_INT8;
      break;

    case DXGI_FORMAT_R8G8B8A8_SNORM:
      fmt.image_channel_order = CL_RGBA;
      fmt.image_channel_data_type = CL_SNORM_INT8;
      break;

    case DXGI_FORMAT_R8G8B8A8_SINT:
      fmt.image_channel_order = CL_RGBA;
      fmt.image_channel_data_type = CL_SIGNED_INT8;
      break;

    case DXGI_FORMAT_R16G16_TYPELESS:
      fmt.image_channel_order = CL_RG;
      break;

    case DXGI_FORMAT_R16G16_FLOAT:
      fmt.image_channel_order = CL_RG;
      fmt.image_channel_data_type = CL_HALF_FLOAT;
      break;

    case DXGI_FORMAT_R16G16_UNORM:
      fmt.image_channel_order = CL_RG;
      fmt.image_channel_data_type = CL_UNORM_INT16;
      break;

    case DXGI_FORMAT_R16G16_UINT:
      fmt.image_channel_order = CL_RG;
      fmt.image_channel_data_type = CL_UNSIGNED_INT16;
      break;

    case DXGI_FORMAT_R16G16_SNORM:
      fmt.image_channel_order = CL_RG;
      fmt.image_channel_data_type = CL_SNORM_INT16;
      break;

    case DXGI_FORMAT_R16G16_SINT:
      fmt.image_channel_order = CL_RG;
      fmt.image_channel_data_type = CL_SIGNED_INT16;
      break;

    case DXGI_FORMAT_R32_TYPELESS:
      fmt.image_channel_order = CL_R;
      break;

    case DXGI_FORMAT_D32_FLOAT:
      break;

    case DXGI_FORMAT_R32_FLOAT:
      fmt.image_channel_order = CL_R;
      fmt.image_channel_data_type = CL_FLOAT;
      break;

    case DXGI_FORMAT_R32_UINT:
      fmt.image_channel_order = CL_R;
      fmt.image_channel_data_type = CL_UNSIGNED_INT32;
      break;

    case DXGI_FORMAT_R32_SINT:
      fmt.image_channel_order = CL_R;
      fmt.image_channel_data_type = CL_SIGNED_INT32;
      break;

    case DXGI_FORMAT_R24G8_TYPELESS:
      fmt.image_channel_order = CL_RG;
      break;

    case DXGI_FORMAT_D24_UNORM_S8_UINT:
      break;

    case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
      break;

    case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
      break;

    case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
      break;

    case DXGI_FORMAT_R8G8_B8G8_UNORM:
      fmt.image_channel_data_type = CL_UNORM_INT8;
      break;

    case DXGI_FORMAT_G8R8_G8B8_UNORM:
      fmt.image_channel_data_type = CL_UNORM_INT8;
      break;

    case DXGI_FORMAT_B8G8R8A8_UNORM:
      fmt.image_channel_order = CL_BGRA;
      fmt.image_channel_data_type = CL_UNORM_INT8;
      break;

    case DXGI_FORMAT_B8G8R8X8_UNORM:
      fmt.image_channel_data_type = CL_UNORM_INT8;
      break;

    case DXGI_FORMAT_R8G8_TYPELESS:
      fmt.image_channel_order = CL_RG;
      break;

    case DXGI_FORMAT_R8G8_UNORM:
      fmt.image_channel_order = CL_RG;
      fmt.image_channel_data_type = CL_UNORM_INT8;
      break;

    case DXGI_FORMAT_R8G8_UINT:
      fmt.image_channel_order = CL_RG;
      fmt.image_channel_data_type = CL_UNSIGNED_INT8;
      break;

    case DXGI_FORMAT_R8G8_SNORM:
      fmt.image_channel_order = CL_RG;
      fmt.image_channel_data_type = CL_SNORM_INT8;
      break;

    case DXGI_FORMAT_R8G8_SINT:
      fmt.image_channel_order = CL_RG;
      fmt.image_channel_data_type = CL_SIGNED_INT8;
      break;

    case DXGI_FORMAT_R16_TYPELESS:
      fmt.image_channel_order = CL_R;
      break;

    case DXGI_FORMAT_R16_FLOAT:
      fmt.image_channel_order = CL_R;
      fmt.image_channel_data_type = CL_HALF_FLOAT;
      break;

    case DXGI_FORMAT_D16_UNORM:
      fmt.image_channel_data_type = CL_UNORM_INT16;
      break;

    case DXGI_FORMAT_R16_UNORM:
      fmt.image_channel_order = CL_R;
      fmt.image_channel_data_type = CL_UNORM_INT16;
      break;

    case DXGI_FORMAT_R16_UINT:
      fmt.image_channel_order = CL_R;
      fmt.image_channel_data_type = CL_UNSIGNED_INT16;
      break;

    case DXGI_FORMAT_R16_SNORM:
      fmt.image_channel_order = CL_R;
      fmt.image_channel_data_type = CL_SNORM_INT16;
      break;

    case DXGI_FORMAT_R16_SINT:
      fmt.image_channel_order = CL_R;
      fmt.image_channel_data_type = CL_SIGNED_INT16;
      break;

    case DXGI_FORMAT_B5G6R5_UNORM:
      fmt.image_channel_data_type = CL_UNORM_SHORT_565;
      break;

    case DXGI_FORMAT_B5G5R5A1_UNORM:
      fmt.image_channel_order = CL_BGRA;
      break;

    case DXGI_FORMAT_R8_TYPELESS:
      fmt.image_channel_order = CL_R;
      break;

    case DXGI_FORMAT_R8_UNORM:
      fmt.image_channel_order = CL_R;
      fmt.image_channel_data_type = CL_UNORM_INT8;
      break;

    case DXGI_FORMAT_R8_UINT:
      fmt.image_channel_order = CL_R;
      fmt.image_channel_data_type = CL_UNSIGNED_INT8;
      break;

    case DXGI_FORMAT_R8_SNORM:
      fmt.image_channel_order = CL_R;
      fmt.image_channel_data_type = CL_SNORM_INT8;
      break;

    case DXGI_FORMAT_R8_SINT:
      fmt.image_channel_order = CL_R;
      fmt.image_channel_data_type = CL_SIGNED_INT8;
      break;

    case DXGI_FORMAT_A8_UNORM:
      fmt.image_channel_order = CL_A;
      fmt.image_channel_data_type = CL_UNORM_INT8;
      break;

    case DXGI_FORMAT_R1_UNORM:
      fmt.image_channel_order = CL_R;
      break;

    case DXGI_FORMAT_BC1_TYPELESS:
    case DXGI_FORMAT_BC1_UNORM:
    case DXGI_FORMAT_BC1_UNORM_SRGB:
    case DXGI_FORMAT_BC2_TYPELESS:
    case DXGI_FORMAT_BC2_UNORM:
    case DXGI_FORMAT_BC2_UNORM_SRGB:
    case DXGI_FORMAT_BC3_TYPELESS:
    case DXGI_FORMAT_BC3_UNORM:
    case DXGI_FORMAT_BC3_UNORM_SRGB:
    case DXGI_FORMAT_BC4_TYPELESS:
    case DXGI_FORMAT_BC4_UNORM:
    case DXGI_FORMAT_BC4_SNORM:
    case DXGI_FORMAT_BC5_TYPELESS:
    case DXGI_FORMAT_BC5_UNORM:
    case DXGI_FORMAT_BC5_SNORM:
      break;
    case DXGI_FORMAT_NV12:
      fmt.image_channel_order = CL_R;
      fmt.image_channel_data_type = CL_UNSIGNED_INT8;
      if (plane == 1) {
        fmt.image_channel_order = CL_RG;
      }
      break;
    case DXGI_FORMAT_P010:
        fmt.image_channel_order = CL_R;
        fmt.image_channel_data_type = CL_UNSIGNED_INT16;
        if (plane == 1) {
            fmt.image_channel_order = CL_RG;
        }
        break;
    default:
      _ASSERT(FALSE);
      break;
  }

  return fmt;
}

}  // namespace amd

#endif  //_WIN32
