////////////////////////////////////////////////////////////////////////////////
//
// Copyright 2014 ADVANCED MICRO DEVICES, INC.
//
// AMD is granting you permission to use this software and documentation(if any)
// (collectively, the "Materials") pursuant to the terms and conditions of the
// Software License Agreement included with the Materials.If you do not have a
// copy of the Software License Agreement, contact your AMD representative for a
// copy.
//
// You agree that you will not reverse engineer or decompile the Materials, in
// whole or in part, except as allowed by applicable law.
//
// WARRANTY DISCLAIMER : THE SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND.AMD DISCLAIMS ALL WARRANTIES, EXPRESS, IMPLIED, OR STATUTORY,
// INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE, TITLE, NON - INFRINGEMENT, THAT THE
// SOFTWARE WILL RUN UNINTERRUPTED OR ERROR - FREE OR WARRANTIES ARISING FROM
// CUSTOM OF TRADE OR COURSE OF USAGE.THE ENTIRE RISK ASSOCIATED WITH THE USE OF
// THE SOFTWARE IS ASSUMED BY YOU.Some jurisdictions do not allow the exclusion
// of implied warranties, so the above exclusion may not apply to You.
//
// LIMITATION OF LIABILITY AND INDEMNIFICATION : AMD AND ITS LICENSORS WILL NOT,
// UNDER ANY CIRCUMSTANCES BE LIABLE TO YOU FOR ANY PUNITIVE, DIRECT,
// INCIDENTAL, INDIRECT, SPECIAL OR CONSEQUENTIAL DAMAGES ARISING FROM USE OF
// THE SOFTWARE OR THIS AGREEMENT EVEN IF AMD AND ITS LICENSORS HAVE BEEN
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.In no event shall AMD's total
// liability to You for all damages, losses, and causes of action (whether in
// contract, tort (including negligence) or otherwise) exceed the amount of $100
// USD.  You agree to defend, indemnify and hold harmless AMD and its licensors,
// and any of their directors, officers, employees, affiliates or agents from
// and against any and all loss, damage, liability and other expenses (including
// reasonable attorneys' fees), resulting from Your use of the Software or
// violation of the terms and conditions of this Agreement.
//
// U.S.GOVERNMENT RESTRICTED RIGHTS : The Materials are provided with
// "RESTRICTED RIGHTS." Use, duplication, or disclosure by the Government is
// subject to the restrictions as set forth in FAR 52.227 - 14 and DFAR252.227 -
// 7013, et seq., or its successor.Use of the Materials by the Government
// constitutes acknowledgement of AMD's proprietary rights in them.
//
// EXPORT RESTRICTIONS: The Materials may be subject to export restrictions as
//                      stated in the Software License Agreement.
//
////////////////////////////////////////////////////////////////////////////////

// AMD specific HSA backend.

#ifndef HSA_RUNTIME_CORE_INC_AMD_GPU_AGENT_H_
#define HSA_RUNTIME_CORE_INC_AMD_GPU_AGENT_H_

#include <vector>

#include "core/inc/runtime.h"
#include "core/inc/agent.h"
#include "core/inc/signal.h"
#include "core/inc/thunk.h"
#include "core/util/small_heap.h"
#include "core/util/locks.h"

namespace amd {

struct ScratchInfo {
  void* queue_base;
  size_t size;
  size_t size_per_thread;
};

class GpuAgentInt : public core::Agent {
 public:
  GpuAgentInt() : core::Agent(core::Agent::DeviceType::kAmdGpuDevice) {}
  virtual bool memory_type(hsa_amd_memory_type_t type) = 0;
  virtual hsa_amd_memory_type_t memory_type() const = 0;
  virtual void TranslateTime(core::Signal* signal,
                             hsa_amd_dispatch_time_t& time) = 0;
};

class GpuAgent : public GpuAgentInt {
 public:
  GpuAgent(HSAuint32 node, const HsaNodeProperties& node_props,
           const std::vector<HsaCacheProperties>& cache_props);

  ~GpuAgent();

  void RegisterMemoryProperties(const HsaMemoryProperties properties);

  hsa_status_t IterateRegion(hsa_status_t (*callback)(hsa_region_t region,
                                                      void* data),
                             void* data) const;

  hsa_status_t GetInfo(hsa_agent_info_t attribute, void* value) const;

  /// @brief Api to create an Aql queue
  ///
  /// @param size Size of Queue in terms of Aql packet size
  ///
  /// @param type of Queue Single Writer or Multiple Writer
  ///
  /// @param callback Callback function to register in case Quee
  /// encounters an error
  ///
  /// @param service_queue Pointer to a service queue
  ///
  /// @parm queue Output parameter updated with a pointer to the
  /// queue being created
  ///
  /// @return hsa_status
  hsa_status_t QueueCreate(size_t size, hsa_queue_type_t type,
                           core::HsaEventCallback callback,
                           const hsa_queue_t* service_queue,
                           core::Queue** queue);

  void AcquireQueueScratch(ScratchInfo &scratch)
  {
	  if(scratch.size==0)
	  {
		  scratch.size=queue_scratch_len_;
		  scratch.size_per_thread=scratch_per_thread_;
	  }
	  ScopedAcquire<KernelMutex> lock(&sclock_);
	  scratch.queue_base = scratch_pool_.alloc(scratch.size);
  }

  void ReleaseQueueScratch(void* base) {
	  ScopedAcquire<KernelMutex> lock(&sclock_);
	  scratch_pool_.free(base);
  }

  void TranslateTime(core::Signal* signal, hsa_amd_dispatch_time_t& time);

  bool memory_type(hsa_amd_memory_type_t type);

  hsa_amd_memory_type_t memory_type() const { return current_memory_type_; }

  __forceinline void SetApe1BaseAndSize(uintptr_t base, size_t size) {
    assert(ape1_base_ == 0 && ape1_size_ == 0 &&
           "There should be only one ape1 segment");
    ape1_base_ = base;
    ape1_size_ = size;
  }

  __forceinline HSAuint32 node_id() const { return node_id_; }

  __forceinline HsaNodeProperties properties() const { return properties_; }

  __forceinline size_t num_cache() const { return cache_props_.size(); }

  __forceinline const HsaCacheProperties& cache_prop(int idx) const {
    return cache_props_[idx];
  }

  uint16_t GetMicrocodeVersion() const;

 private:
  static const uint32_t maxAqlSize_ = 0x20000;  // 8MB max

  void SyncClocks();

  const HSAuint32 node_id_;

  const HsaNodeProperties properties_;

  std::vector<HsaCacheProperties> cache_props_;

  uintptr_t ape1_base_;

  size_t ape1_size_;

  hsa_amd_memory_type_t current_memory_type_;

  SmallHeap scratch_pool_;

  size_t queue_scratch_len_;

  size_t scratch_per_thread_;

  KernelMutex lock_, sclock_;

  HsaClockCounters t0_, t1_;

  DISALLOW_COPY_AND_ASSIGN(GpuAgent);
};

}  // namespace

#endif  // header guard
