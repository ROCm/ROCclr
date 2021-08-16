 /* Copyright (c) 2008 - 2021 Advanced Micro Devices, Inc.

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

#include "gsl_ctx.h"
#include "GSLDevice.h"

#if defined(ATI_OS_WIN)

#include <D3D11.h>

/**************************************************************************************************************
* Note: ideally the DXX extension interfaces should be mapped from the DXX perforce branch. 
* This means CAL client spec will need to change to include headers directly from the DXX perforce tree. 
* However, CAL only cares about the DXX OpenCL extension interface class. The spec cannot change
* without notification. So it is safe to use a local copy of the relevant DXX extension interface classes.
**************************************************************************************************************/
#include "DxxOpenCLInteropExt.h"

static bool
queryD3D11DeviceGPUMask(ID3D11Device* pd3d11Device, UINT* pd3d11DeviceGPUMask)
{
    HMODULE             hDLL = NULL;
    IAmdDxExt*          pExt = NULL;
    IAmdDxExtCLInterop* pCLExt = NULL;
    PFNAmdDxExtCreate11 AmdDxExtCreate11;
    HRESULT             hr = S_OK;

    // Get a handle to the DXX DLL with extension API support
#if defined _WIN64
    static const CHAR dxxModuleName[13] = "atidxx64.dll";
#else
    static const CHAR dxxModuleName[13] = "atidxx32.dll";
#endif

    hDLL = GetModuleHandle(dxxModuleName);

    if (hDLL == NULL)
    {
        hr = E_FAIL;
    }

    // Get the exported AmdDxExtCreate() function pointer
    if (SUCCEEDED(hr))
    {
        AmdDxExtCreate11 = reinterpret_cast<PFNAmdDxExtCreate11>(GetProcAddress(hDLL, "AmdDxExtCreate11"));
        if (AmdDxExtCreate11 == NULL)
        {
            hr = E_FAIL;
        }
    }

    // Create the extension object
    if (SUCCEEDED(hr))
    {
        hr = AmdDxExtCreate11(pd3d11Device, &pExt);
    }

    // Get the extension version information
    if (SUCCEEDED(hr))
    {
        AmdDxExtVersion extVersion;
        hr = pExt->GetVersion(&extVersion);

        if (extVersion.majorVersion == 0)
        {
            hr = E_FAIL;
        }
    }

    // Get the OpenCL Interop interface
    if (SUCCEEDED(hr))
    {
        pCLExt = static_cast<IAmdDxExtCLInterop*>(pExt->GetExtInterface(AmdDxExtCLInteropID));
        if (pCLExt != NULL)
        {
            // Get the GPU mask using the CL Interop extension.
            pCLExt->QueryInteropGpuMask(pd3d11DeviceGPUMask);
        }
        else
        {
            hr = E_FAIL;
        }
    }

    if (pCLExt != NULL)
    {
        pCLExt->Release();
    }

    if (pExt != NULL)
    {
        pExt->Release();
    }    

    return (SUCCEEDED(hr));
}

bool
CALGSLDevice::associateD3D11Device(void* d3d11Device)
{
    bool canInteroperate = false;

    LUID calDevAdapterLuid = {0, 0};
    UINT calDevChainBitMask = 0;
    UINT d3d11DeviceGPUMask = 0;

    ID3D11Device* pd3d11Device = static_cast<ID3D11Device*>(d3d11Device);

    IDXGIDevice* pDXGIDevice;
    pd3d11Device->QueryInterface(__uuidof(IDXGIDevice), (void **)&pDXGIDevice);

    IDXGIAdapter* pDXGIAdapter;
    pDXGIDevice->GetAdapter(&pDXGIAdapter);

    DXGI_ADAPTER_DESC adapterDesc;
    pDXGIAdapter->GetDesc(&adapterDesc);

    // match the adapter
    if (m_adp->getMVPUinfo(&calDevAdapterLuid, &calDevChainBitMask))
    {
        canInteroperate = ((calDevAdapterLuid.HighPart == adapterDesc.AdapterLuid.HighPart) &&
                           (calDevAdapterLuid.LowPart == adapterDesc.AdapterLuid.LowPart));
    }

    // match the chain ID
    if (canInteroperate)
    {
        if (queryD3D11DeviceGPUMask(pd3d11Device, &d3d11DeviceGPUMask))
        {
            canInteroperate = (calDevChainBitMask & d3d11DeviceGPUMask) != 0;
        }
        else
        {
            // special handling for Intel iGPU + AMD dGPU in LDA mode (only occurs on a PX platform) where 
            // the D3D11Device object is created on the Intel iGPU and passed to AMD dGPU (secondary) to interoperate. 
            if (calDevChainBitMask > 1)
            {
                canInteroperate = false;
            }
        }
    }

    pDXGIDevice->Release();
    pDXGIAdapter->Release();

    return canInteroperate;
}

#else // !ATI_OS_WIN

bool
CALGSLDevice::associateD3D11Device(void* d3d11Device)
{
    return false;
}

#endif // !ATI_OS_WIN
