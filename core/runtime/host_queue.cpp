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

#include "core/inc/host_queue.h"

#include "core/inc/runtime.h"
#include "core/util/utils.h"

namespace core {
HostQueue::HostQueue(uint32_t ring_size) : size_(ring_size), active_(false) {
  hsa_memory_register(this, sizeof(HostQueue));

  ring_ = _aligned_malloc(size_ * sizeof(hsa_agent_dispatch_packet_t),
                          kRingAlignment);
  if (ring_ == NULL) return;
  hsa_memory_register(ring_, size_ * sizeof(hsa_agent_dispatch_packet_t));
  MAKE_NAMED_SCOPE_GUARD(ring_guard, [&]() {
    hsa_memory_deregister(ring_, size_ * sizeof(hsa_agent_dispatch_packet_t));
    _aligned_free(ring_);
  });

  if (hsa_signal_create(-1, 0, NULL, &signal_) != HSA_STATUS_SUCCESS) return;

  amd_queue_.hsa_queue.base_address = reinterpret_cast<uint64_t>(ring_);
  amd_queue_.hsa_queue.size = size_;
#ifdef HSA_LARGE_MODEL
  amd_queue_.is_ptr64 = 1;
#else
  amd_queue_.is_ptr64 = 0;
#endif
  amd_queue_.write_dispatch_id = amd_queue_.read_dispatch_id = 0;
  amd_queue_.hsa_queue.doorbell_signal = signal_;
  amd_queue_.hsa_queue.service_queue = NULL;
  amd_queue_.hsa_queue.id = Runtime::runtime_singleton_->GetQueueId();
  amd_queue_.hsa_queue.queue_type = HSA_QUEUE_TYPE_MULTI;
  amd_queue_.hsa_queue.queue_features = HSA_QUEUE_FEATURE_AGENT_DISPATCH;
  amd_queue_.enable_profiling = 0;

  active_ = true;
  ring_guard.Dismiss();
}

HostQueue::~HostQueue() {
  hsa_signal_destroy(signal_);
  hsa_memory_deregister(ring_, size_ * sizeof(hsa_agent_dispatch_packet_t));
  _aligned_free(ring_);
  hsa_memory_deregister(this, sizeof(HostQueue));
}

}  // namespace core