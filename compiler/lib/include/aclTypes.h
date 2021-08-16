/* Copyright (c) 2012 - 2021 Advanced Micro Devices, Inc.

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

#ifndef _ACL_API_TYPES_0_8_H_
#define _ACL_API_TYPES_0_8_H_
#include "aclDefs.h"
#include <stdint.h>
#include <stddef.h>

// Typedefs that always point to the most recent versions of the objects.
typedef struct _acl_md_arg_type_0_8       aclArgData;
typedef struct _acl_md_printf_fmt_0_8     aclPrintfFmt;
typedef struct _acl_metadata_0_8          aclMetadata;
typedef struct _acl_device_caps_rec_0_8   aclDevCaps;
typedef struct _acl_target_info_rec_0_8   aclTargetInfo;
typedef struct _acl_bif_rec_0_8_1         aclBinary;
typedef struct _acl_binary_opts_rec_0_8_1 aclBinaryOptions;
typedef struct _acl_compiler_rec_0_8_1      aclCompiler;
typedef struct _acl_compiler_opts_rec_0_8_1 aclCompilerOptions;
typedef struct _acl_options_0_8*          aclOptions;  // Opaque pointer to amd::Options
typedef struct _acl_binary_0_8*           aclBIF; // Opaque pointer to bifbase
typedef struct _acl_common_loader_rec_0_8 aclCommonLoader;
typedef struct _acl_cl_loader_rec_0_8     aclCLLoader;
typedef struct _acl_sc_loader_rec_0_8     aclSCLoader;
typedef struct _acl_fe_loader_rec_0_8     aclFELoader;
typedef struct _acl_link_loader_rec_0_8   aclLinkLoader;
typedef struct _acl_opt_loader_rec_0_8    aclOptLoader;
typedef struct _acl_cg_loader_rec_0_8     aclCGLoader;
typedef struct _acl_be_loader_rec_0_8     aclBELoader;
typedef struct _acl_llvm_module_0_8*      aclModule; // Opaque pointer to llvm::Module
typedef struct _acl_llvm_context_0_8*     aclContext; // Opaque pointer to llvm::Context
typedef struct _acl_loader_data_0_8*      aclLoaderData; // Opaque pointer to loader data

#include "aclEnums.h"
// Typedefs for enumerations
typedef enum   _acl_error_enum_0_8         acl_error;
typedef enum   _comp_device_caps_enum_0_8  compDeviceCaps;
typedef enum   _comp_opt_settings_enum_0_8 compOptSettings;
typedef enum   _acl_dev_type_enum_0_8      aclDevType;
typedef enum   _acl_cl_version_enum_0_8    aclCLVersion;
typedef enum   _acl_type_enum_0_8          aclType;
typedef enum   _rt_query_types_enum_0_8    aclQueryType;
typedef enum   _rt_gpu_caps_enum_0_8       aclGPUCaps;
typedef enum   _rt_gpu_resource_enum_0_8   aclGPUResource;
typedef enum   _rt_gpu_mem_sizes_enum_0_8  aclGPUMemSizes;
typedef enum   _acl_arg_type_enum_0_8      aclArgType;
typedef enum   _acl_data_type_enum_0_8     aclArgDataType;
typedef enum   _acl_memory_type_enum_0_8   aclMemoryType;
typedef enum   _acl_access_type_enum_0_8   aclAccessType;
typedef enum   _bif_version_enum_0_8       aclBIFVersion;
typedef enum   _bif_platform_enum_0_8      aclPlatform;
typedef enum   _bif_sections_enum_0_8      aclSections;
typedef enum   _acl_loader_type_enum_0_8   aclLoaderType;
typedef enum   _acl_binary_image_type_enum_0_8   aclBinaryImageType;

#include "aclFunctors.h"
// Typedefs for function pointers
typedef aclLogFunction_0_8 aclLogFunction;
typedef InsertSec_0_8      InsertSec;
typedef RemoveSec_0_8      RemoveSec;
typedef ExtractSec_0_8     ExtractSec;
typedef InsertSym_0_8      InsertSym;
typedef RemoveSym_0_8      RemoveSym;
typedef ExtractSym_0_8     ExtractSym;
typedef QueryInfo_0_8      QueryInfo;
typedef Compile_0_8        Compile;
typedef Link_0_8           Link;
typedef AddDbgArg_0_8      AddDbgArg;
typedef RemoveDbgArg_0_8   RemoveDbgArg;
typedef SetupLoaderObject_0_8 SetupLoaderObject;
typedef CompLog_0_8        CompLog;
typedef RetrieveType_0_8   RetrieveType;
typedef SetType_0_8        SetType;
typedef ConvertType_0_8    ConvertType;
typedef Disassemble_0_8    Disassemble;
typedef GetDevBinary_0_8   GetDevBinary;
typedef LoaderInit_0_8     LoaderInit;
typedef LoaderFini_0_8     LoaderFini;
typedef FEToIR_0_8         FEToIR;
typedef SourceToISA_0_8    SourceToISA;
typedef IRPhase_0_8        IRPhase;
typedef LinkPhase_0_8      LinkPhase;
typedef CGPhase_0_8        CGPhase;
typedef DisasmISA_0_8      DisasmISA;
typedef AllocFunc_0_8      AllocFunc;
typedef FreeFunc_0_8       FreeFunc;
typedef JITObjectImageCreate_0_8 JITObjectImageCreate;
typedef JITObjectImageCopy_0_8 JITObjectImageCopy;
typedef JITObjectImageDestroy_0_8 JITObjectImageDestroy;
typedef JITObjectImageSize_0_8 JITObjectImageSize;
typedef JITObjectImageData_0_8 JITObjectImageData;
typedef JITObjectImageFinalize_0_8 JITObjectImageFinalize;
typedef JITObjectImageGetGlobalsSize_0_8 JITObjectImageGetGlobalsSize;
typedef JITSymbolCallback_0_8 JITSymbolCallback;
typedef JITObjectImageIterateSymbols_0_8 JITObjectImageIterateSymbols;
typedef JITObjectImageDisassembleKernel_0_8 JITObjectImageDisassembleKernel;

#include "aclStructs.h"

#endif // _CL_API_TYPES_0_8_H_
