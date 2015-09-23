/******************************************************************************
* University of Illinois / NCSA
* Open Source License
*
* Copyright(c) 2011 - 2015  Advanced Micro Devices, Inc.
* All rights reserved.
*
* Developed by:
* Advanced Micro Devices, Inc.
* www.amd.com
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files(the "Software"), to deal
* with the Software without restriction, including without limitation the
* rights to use, copy, modify, merge, publish, distribute, sublicense, and /
* or sell copies of the Software, and to permit persons to whom the Software
* is furnished to do so, subject to the following conditions:
*
*     Redistributions of source code must retain the above copyright notice,
* this list of conditions and the following disclaimers.
*
*     Redistributions in binary form must reproduce the above copyright notice,
* this list of conditions and the following disclaimers in the documentation
* and / or other materials provided with the distribution.
*
*     Neither the names of Advanced Micro Devices, Inc, nor the
mes of its
* contributors may be used to endorse or promote products derived from this
* Software without specific prior written permission.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
* CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS WITH
* THE SOFTWARE.
******************************************************************************/

#include <string>
#include <vector>
#include <iostream>
#ifdef _WIN32
#include <malloc.h>
#else // _WIN32
#include <cstdlib>
#endif // _WIN32
#include "amd_hsa_kernel_code.h"
#include "hsa.h"
#include "hsa_ext_finalize.h"

#ifndef AMD_HSA_CODE_UTIL_HPP_
#define AMD_HSA_CODE_UTIL_HPP_

namespace amd {
namespace hsa {

std::string HsaSymbolKindToString(hsa_symbol_kind_t kind);
std::string HsaSymbolLinkageToString(hsa_symbol_linkage_t linkage);
std::string HsaVariableAllocationToString(hsa_variable_allocation_t allocation);
std::string HsaVariableSegmentToString(hsa_variable_segment_t segment);
std::string AmdMachineKindToString(amd_machine_kind16_t machine);
std::string AmdFloatRoundModeToString(amd_float_round_mode_t round_mode);
std::string AmdFloatDenormModeToString(amd_float_denorm_mode_t denorm_mode);
std::string AmdSystemVgprWorkitemIdToString(amd_system_vgpr_workitem_id_t system_vgpr_workitem_id);
std::string AmdElementByteSizeToString(amd_element_byte_size_t element_byte_size);
std::string AmdExceptionKindToString(amd_exception_kind16_t exceptions);

void PrintAmdKernelCode(std::ostream& out, const amd_kernel_code_t *akc);
void PrintAmdComputePgmRsrcOne(std::ostream& out, amd_compute_pgm_rsrc_one32_t compute_pgm_rsrc1);
void PrintAmdComputePgmRsrcTwo(std::ostream& out, amd_compute_pgm_rsrc_two32_t compute_pgm_rsrc2);
void PrintAmdKernelCodeProperties(std::ostream& out, amd_kernel_code_properties32_t kernel_code_properties);
void PrintAmdControlDirectives(std::ostream& out, const amd_control_directives_t &control_directives);

// \todo kzhuravl 8/10/2015 rename.
const char* hsaerr2str(hsa_status_t status);
bool ReadFileIntoBuffer(const std::string& filename, std::vector<char>& buffer);

// Create new empty temporary file that will be deleted when closed.
int OpenTempFile(const char* prefix);
void CloseTempFile(int fd);

// Helper function that allocates an aligned memory.
inline void*
alignedMalloc(size_t size, size_t alignment)
{
#if defined(_WIN32)
  return ::_aligned_malloc(size, alignment);
#else
  void * ptr = NULL;
  alignment = (std::max)(alignment, sizeof(void*));
  if (0 == ::posix_memalign(&ptr, alignment, size)) {
    return ptr;
  }
  return NULL;
#endif
}

// Helper function that frees an aligned memory.
inline void
alignedFree(void *ptr)
{
#if defined(_WIN32)
  ::_aligned_free(ptr);
#else
  free(ptr);
#endif
}

}
}

#endif // AMD_HSA_CODE_UTIL_HPP_
