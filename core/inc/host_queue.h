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

#ifndef HSA_RUNTIME_CORE_INC_HOST_QUEUE_H_
#define HSA_RUNTIME_CORE_INC_HOST_QUEUE_H_

#include "core/inc/runtime.h"
#include "core/inc/queue.h"

namespace core {
class HostQueue : public Queue {
 public:
  HostQueue(uint32_t ring_size);

  ~HostQueue();

  hsa_status_t Inactivate() { return HSA_STATUS_SUCCESS; }

  uint64_t LoadReadIndexAcquire() {
    return atomic::Load(&amd_queue_.read_dispatch_id,
                        std::memory_order_acquire);
  }

  uint64_t LoadReadIndexRelaxed() {
    return atomic::Load(&amd_queue_.read_dispatch_id,
                        std::memory_order_relaxed);
  }

  uint64_t LoadWriteIndexAcquire() {
    return atomic::Load(&amd_queue_.write_dispatch_id,
                        std::memory_order_acquire);
  }

  uint64_t LoadWriteIndexRelaxed() {
    return atomic::Load(&amd_queue_.write_dispatch_id,
                        std::memory_order_relaxed);
  }

  void StoreReadIndexRelaxed(uint64_t value) {
    atomic::Store(&amd_queue_.read_dispatch_id, value,
                  std::memory_order_relaxed);
  }

  void StoreReadIndexRelease(uint64_t value) {
    atomic::Store(&amd_queue_.read_dispatch_id, value,
                  std::memory_order_release);
  }

  void StoreWriteIndexRelaxed(uint64_t value) {
    atomic::Store(&amd_queue_.write_dispatch_id, value,
                  std::memory_order_relaxed);
  }

  void StoreWriteIndexRelease(uint64_t value) {
    atomic::Store(&amd_queue_.write_dispatch_id, value,
                  std::memory_order_release);
  }

  uint64_t CasWriteIndexAcqRel(uint64_t expected, uint64_t value) {
    return atomic::Cas(&amd_queue_.write_dispatch_id, value, expected,
                       std::memory_order_acq_rel);
  }

  uint64_t CasWriteIndexAcquire(uint64_t expected, uint64_t value) {
    return atomic::Cas(&amd_queue_.write_dispatch_id, value, expected,
                       std::memory_order_acquire);
  }

  uint64_t CasWriteIndexRelaxed(uint64_t expected, uint64_t value) {
    return atomic::Cas(&amd_queue_.write_dispatch_id, value, expected,
                       std::memory_order_relaxed);
  }

  uint64_t CasWriteIndexRelease(uint64_t expected, uint64_t value) {
    return atomic::Cas(&amd_queue_.write_dispatch_id, value, expected,
                       std::memory_order_release);
  }

  uint64_t AddWriteIndexAcqRel(uint64_t value) {
    return atomic::Add(&amd_queue_.write_dispatch_id, value,
                       std::memory_order_acq_rel);
  }

  uint64_t AddWriteIndexAcquire(uint64_t value) {
    return atomic::Add(&amd_queue_.write_dispatch_id, value,
                       std::memory_order_acquire);
  }

  uint64_t AddWriteIndexRelaxed(uint64_t value) {
    return atomic::Add(&amd_queue_.write_dispatch_id, value,
                       std::memory_order_relaxed);
  }

  uint64_t AddWriteIndexRelease(uint64_t value) {
    return atomic::Add(&amd_queue_.write_dispatch_id, value,
                       std::memory_order_release);
  }

  bool active() const { return active_; }

  void* operator new(size_t size) {
    return _aligned_malloc(size, HSA_QUEUE_ALIGN_BYTES);
  }

  void* operator new(size_t size, void* ptr) { return ptr; }

  void operator delete(void* ptr) { _aligned_free(ptr); }

  void operator delete(void*, void*) {}

 private:
  static const size_t kRingAlignment = 256;
  const uint32_t size_;
  bool active_;
  void* ring_;
  hsa_signal_t signal_;

  DISALLOW_COPY_AND_ASSIGN(HostQueue);
};
}  // namespace core
#endif  // header guard
