///////////////////////////////////////////////////////////////////////////////
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

// HSA runtime C++ interface file.

#ifndef HSA_RUNTME_CORE_INC_AGENT_H_
#define HSA_RUNTME_CORE_INC_AGENT_H_

#include <assert.h>

#include <vector>

#include "core/inc/runtime.h"
#include "core/inc/checked.h"
#include "core/inc/queue.h"
#include "core/inc/memory_region.h"
#include "core/util/utils.h"
#include "core/runtime/compute_capability.hpp"

namespace core {
typedef void (*HsaEventCallback)(hsa_status_t status, hsa_queue_t* source,
                                 void* data);

class MemoryRegion;

/*
Agent is intended to be an pure interface class and may be wrapped or replaced
by tools.
All funtions other than Convert, device_type, and public_handle must be virtual.
*/
class Agent : public Checked<0xF6BC25EB17E6F917> {
 public:
  // Lightweight RTTI for vendor specific implementations.
  enum DeviceType { kAmdGpuDevice = 0, kAmdCpuDevice = 1, kUnknownDevice = 2 };

  explicit Agent(DeviceType type)
      : compute_capability_(), device_type_(uint32_t(type)) {
    public_handle_ = Convert(this);
  }
  explicit Agent(uint32_t type) : compute_capability_(), device_type_(type) {
    public_handle_ = Convert(this);
  }

  virtual ~Agent() {}

  // Convert this object into hsa_agent_t.
  static __forceinline hsa_agent_t Convert(Agent* agent) {
    const hsa_agent_t agent_handle = {
        static_cast<uint64_t>(reinterpret_cast<uintptr_t>(agent))};
    return agent_handle;
  }

  static __forceinline const hsa_agent_t Convert(const Agent* agent) {
    const hsa_agent_t agent_handle = {
        static_cast<uint64_t>(reinterpret_cast<uintptr_t>(agent))};
    return agent_handle;
  }

  // Convert hsa_agent_t into Agent *.
  static __forceinline Agent* Convert(hsa_agent_t agent) {
    return reinterpret_cast<Agent*>(agent.handle);
  }

  virtual hsa_status_t DmaCopy(void* dst, const void* src, size_t size) {
    return HSA_STATUS_ERROR;
  }

  virtual hsa_status_t IterateRegion(
      hsa_status_t (*callback)(hsa_region_t region, void* data),
      void* data) const = 0;

  virtual hsa_status_t QueueCreate(size_t size, hsa_queue_type_t queue_type,
                                   HsaEventCallback event_callback, void* data,
                                   uint32_t private_segment_size,
                                   uint32_t group_segment_size,
                                   Queue** queue) = 0;

  // Translate vendor specific agent properties into HSA agent attribute.
  virtual hsa_status_t GetInfo(hsa_agent_info_t attribute,
                               void* value) const = 0;

  virtual const std::vector<const core::MemoryRegion*>& regions() const = 0;

  // For lightweight RTTI
  __forceinline uint32_t device_type() const { return device_type_; }

  __forceinline hsa_agent_t public_handle() const { return public_handle_; }

  // Get agent's compute capability value
  __forceinline const ComputeCapability& compute_capability() const {
    return compute_capability_;
  }

 protected:
  // Intention here is to have a polymorphic update procedure for public_handle_
  // which is callable on any Agent* but only from some class dervied from
  // Agent*.  do_set_public_handle should remain protected or private in all
  // derived types.
  static __forceinline void set_public_handle(Agent* agent,
                                              hsa_agent_t handle) {
    agent->do_set_public_handle(handle);
  }
  virtual void do_set_public_handle(hsa_agent_t handle) {
    public_handle_ = handle;
  }
  hsa_agent_t public_handle_;
  ComputeCapability compute_capability_;

 private:
  // Forbid copying and moving of this object
  DISALLOW_COPY_AND_ASSIGN(Agent);

  const uint32_t device_type_;
};
}  // namespace core

#endif  // header guard
