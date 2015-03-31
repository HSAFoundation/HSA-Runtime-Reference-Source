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

namespace core {
typedef void (*HsaEventCallback)(hsa_status_t status, hsa_queue_t* queue);

class MemoryRegion;

class Agent : public Checked<0xF6BC25EB17E6F917> {
 public:
  // Light weight RTTI for vendor specific implementations.
  enum DeviceType { kAmdGpuDevice, kAmdCpuDevice, kUnknownDevice };

  explicit Agent(DeviceType type) : device_type_(type) {}

  virtual ~Agent() {
    assert((regions_.size() == 0) && ("Region list is not released properly"));
  }

  // Convert this object into hsa_agent_t.
  static __forceinline hsa_agent_t Convert(Agent* agent) {
    return static_cast<hsa_agent_t>(reinterpret_cast<uintptr_t>(agent));
  }

  static __forceinline const hsa_agent_t Convert(const Agent* agent) {
    return static_cast<hsa_agent_t>(reinterpret_cast<uintptr_t>(agent));
  }

  // Convert hsa_agent_t into Agent *.
  static __forceinline Agent* Convert(hsa_agent_t agent) {
    return reinterpret_cast<Agent*>(agent);
  }

  virtual hsa_status_t IterateRegion(
      hsa_status_t (*callback)(hsa_region_t region, void* data),
      void* data) const = 0;

  virtual hsa_status_t QueueCreate(size_t size, hsa_queue_type_t queue_type,
                                   HsaEventCallback event_callback,
                                   const hsa_queue_t* service_queue,
                                   Queue** queue) = 0;

  // Translate vendor specific agent properties into HSA agent attribute.
  virtual hsa_status_t GetInfo(hsa_agent_info_t attribute,
                               void* value) const = 0;

  __forceinline const std::vector<const MemoryRegion*>& regions() const {
    return regions_;
  }

  __forceinline DeviceType device_type() const { return device_type_; }

 protected:
  // Memory regions accessible by this component.
  std::vector<const MemoryRegion*> regions_;

 private:
  // Forbid copying and moving of this object
  DISALLOW_COPY_AND_ASSIGN(Agent);

  const DeviceType device_type_;
};
}  // namespace core

#endif  // header guard
