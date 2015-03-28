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

#ifndef HSA_RUNTIME_CORE_INC_AMD_HW_AQL_COMMAND_PROCESSOR_H_
#define HSA_RUNTIME_CORE_INC_AMD_HW_AQL_COMMAND_PROCESSOR_H_

#include "core/inc/runtime.h"
#include "core/inc/signal.h"
#include "core/inc/queue.h"
#include "core/inc/amd_gpu_agent.h"

namespace amd {
/// @brief Encapsulates HW Aql Command Processor functionality. It
/// provide the interface for things such as Doorbell register, read,
/// write pointers and a buffer.
class HwAqlCommandProcessor : public core::Queue, public core::Signal {
 public:
  // Acquires/releases queue resources and requests HW schedule/deschedule.
  HwAqlCommandProcessor(GpuAgent* agent, size_t req_size_pkts,
                        HSAuint32 node_id, ScratchInfo& scratch,
                        core::HsaEventCallback callback, void* err_data);

  ~HwAqlCommandProcessor();

  /// @brief Indicates if queue is valid or not
  bool IsValid() const { return valid_; }

  /// @brief Queue interfaces
  hsa_status_t Inactivate();

  /// @brief Atomically reads the Read index of with Acquire semantics
  ///
  /// @return uint64_t Value of read index
  uint64_t LoadReadIndexAcquire();

  /// @brief Atomically reads the Read index of with Relaxed semantics
  ///
  /// @return uint64_t Value of read index
  uint64_t LoadReadIndexRelaxed();

  /// @brief Atomically reads the Write index of with Acquire semantics
  ///
  /// @return uint64_t Value of write index
  uint64_t LoadWriteIndexAcquire();

  /// @brief Atomically reads the Write index of with Relaxed semantics
  ///
  /// @return uint64_t Value of write index
  uint64_t LoadWriteIndexRelaxed();

  /// @brief This operation is illegal
  void StoreReadIndexRelaxed(uint64_t value) { assert(false); }

  /// @brief This operation is illegal
  void StoreReadIndexRelease(uint64_t value) { assert(false); }

  /// @brief Atomically writes the Write index of with Relaxed semantics
  ///
  /// @param value New value of write index to update with
  void StoreWriteIndexRelaxed(uint64_t value);

  /// @brief Atomically writes the Write index of with Release semantics
  ///
  /// @param value New value of write index to update with
  void StoreWriteIndexRelease(uint64_t value);

  /// @brief Compares and swaps Write index using Acquire and Release semantics
  ///
  /// @param expected Current value of write index
  ///
  /// @param value Value of new write index
  ///
  /// @return uint64_t Value of write index before the update
  uint64_t CasWriteIndexAcqRel(uint64_t expected, uint64_t value);

  /// @brief Compares and swaps Write index using Acquire semantics
  ///
  /// @param expected Current value of write index
  ///
  /// @param value Value of new write index
  ///
  /// @return uint64_t Value of write index before the update
  uint64_t CasWriteIndexAcquire(uint64_t expected, uint64_t value);

  /// @brief Compares and swaps Write index using Relaxed semantics
  ///
  /// @param expected Current value of write index
  ///
  /// @param value Value of new write index
  ///
  /// @return uint64_t Value of write index before the update
  uint64_t CasWriteIndexRelaxed(uint64_t expected, uint64_t value);

  /// @brief Compares and swaps Write index using Release semantics
  ///
  /// @param expected Current value of write index
  ///
  /// @param value Value of new write index
  ///
  /// @return uint64_t Value of write index before the update
  uint64_t CasWriteIndexRelease(uint64_t expected, uint64_t value);

  /// @brief Updates the Write index using Acquire and Release semantics
  ///
  /// @param value Value of new write index
  ///
  /// @return uint64_t Value of write index before the update
  uint64_t AddWriteIndexAcqRel(uint64_t value);

  /// @brief Updates the Write index using Acquire semantics
  ///
  /// @param value Value of new write index
  ///
  /// @return uint64_t Value of write index before the update
  uint64_t AddWriteIndexAcquire(uint64_t value);

  /// @brief Updates the Write index using Relaxed semantics
  ///
  /// @param value Value of new write index
  ///
  /// @return uint64_t Value of write index before the update
  uint64_t AddWriteIndexRelaxed(uint64_t value);

  /// @brief Updates the Write index using Release semantics
  ///
  /// @param value Value of new write index
  ///
  /// @return uint64_t Value of write index before the update
  uint64_t AddWriteIndexRelease(uint64_t value);

  /// @brief This operation is illegal
  hsa_signal_value_t LoadRelaxed() {
    assert(false);
    return 0;
  }

  /// @brief This operation is illegal
  hsa_signal_value_t LoadAcquire() {
    assert(false);
    return 0;
  }

  /// @brief Update signal value using Relaxed semantics
  void StoreRelaxed(hsa_signal_value_t value);

  /// @brief Update signal value using Release semantics
  void StoreRelease(hsa_signal_value_t value);

  /// @brief This operation is illegal
  hsa_signal_value_t WaitRelaxed(hsa_signal_condition_t condition,
                                 hsa_signal_value_t compare_value,
                                 uint64_t timeout, hsa_wait_state_t wait_hint) {
    assert(false);
    return 0;
  }

  /// @brief This operation is illegal
  hsa_signal_value_t WaitAcquire(hsa_signal_condition_t condition,
                                 hsa_signal_value_t compare_value,
                                 uint64_t timeout, hsa_wait_state_t wait_hint) {
    assert(false);
    return 0;
  }

  /// @brief This operation is illegal
  void AndRelaxed(hsa_signal_value_t value) { assert(false); }

  /// @brief This operation is illegal
  void AndAcquire(hsa_signal_value_t value) { assert(false); }

  /// @brief This operation is illegal
  void AndRelease(hsa_signal_value_t value) { assert(false); }

  /// @brief This operation is illegal
  void AndAcqRel(hsa_signal_value_t value) { assert(false); }

  /// @brief This operation is illegal
  void OrRelaxed(hsa_signal_value_t value) { assert(false); }

  /// @brief This operation is illegal
  void OrAcquire(hsa_signal_value_t value) { assert(false); }

  /// @brief This operation is illegal
  void OrRelease(hsa_signal_value_t value) { assert(false); }

  /// @brief This operation is illegal
  void OrAcqRel(hsa_signal_value_t value) { assert(false); }

  /// @brief This operation is illegal
  void XorRelaxed(hsa_signal_value_t value) { assert(false); }

  /// @brief This operation is illegal
  void XorAcquire(hsa_signal_value_t value) { assert(false); }

  /// @brief This operation is illegal
  void XorRelease(hsa_signal_value_t value) { assert(false); }

  /// @brief This operation is illegal
  void XorAcqRel(hsa_signal_value_t value) { assert(false); }

  /// @brief This operation is illegal
  void AddRelaxed(hsa_signal_value_t value) { assert(false); }

  /// @brief This operation is illegal
  void AddAcquire(hsa_signal_value_t value) { assert(false); }

  /// @brief This operation is illegal
  void AddRelease(hsa_signal_value_t value) { assert(false); }

  /// @brief This operation is illegal
  void AddAcqRel(hsa_signal_value_t value) { assert(false); }

  /// @brief This operation is illegal
  void SubRelaxed(hsa_signal_value_t value) { assert(false); }

  /// @brief This operation is illegal
  void SubAcquire(hsa_signal_value_t value) { assert(false); }

  /// @brief This operation is illegal
  void SubRelease(hsa_signal_value_t value) { assert(false); }

  /// @brief This operation is illegal
  void SubAcqRel(hsa_signal_value_t value) { assert(false); }

  /// @brief This operation is illegal
  hsa_signal_value_t ExchRelaxed(hsa_signal_value_t value) {
    assert(false);
    return 0;
  }

  /// @brief This operation is illegal
  hsa_signal_value_t ExchAcquire(hsa_signal_value_t value) {
    assert(false);
    return 0;
  }

  /// @brief This operation is illegal
  hsa_signal_value_t ExchRelease(hsa_signal_value_t value) {
    assert(false);
    return 0;
  }

  /// @brief This operation is illegal
  hsa_signal_value_t ExchAcqRel(hsa_signal_value_t value) {
    assert(false);
    return 0;
  }

  /// @brief This operation is illegal
  hsa_signal_value_t CasRelaxed(hsa_signal_value_t expected,
                                hsa_signal_value_t value) {
    assert(false);
    return 0;
  }

  /// @brief This operation is illegal
  hsa_signal_value_t CasAcquire(hsa_signal_value_t expected,
                                hsa_signal_value_t value) {
    assert(false);
    return 0;
  }

  /// @brief This operation is illegal
  hsa_signal_value_t CasRelease(hsa_signal_value_t expected,
                                hsa_signal_value_t value) {
    assert(false);
    return 0;
  }

  /// @brief This operation is illegal
  hsa_signal_value_t CasAcqRel(hsa_signal_value_t expected,
                               hsa_signal_value_t value) {
    assert(false);
    return 0;
  }

  /// @brief This operation is illegal
  hsa_signal_value_t* ValueLocation() const {
    assert(false);
    return NULL;
  }

  /// @brief This operation is illegal
  HsaEvent* EopEvent() {
    assert(false);
    return NULL;
  }

  // 64 byte-aligned allocation and release, for Queue::amd_queue_.
  void* operator new(size_t size);
  void* operator new(size_t size, void* ptr) { return ptr; }
  void operator delete(void* ptr);
  void operator delete(void*, void*) {}

 private:
  // (De)allocates and (de)registers ring_buf_.
  void AllocRegisteredRingBuffer(uint32_t queue_size_pkts);
  void FreeRegisteredRingBuffer();

  static bool DynamicScratchHandler(hsa_signal_value_t error_code, void* arg);

  // AQL packet ring buffer
  void* ring_buf_;

  // Size of ring_buf_ allocation.
  // This may be larger than (amd_queue_.hsa_queue.size * sizeof(AqlPacket)).
  uint32_t ring_buf_alloc_bytes_;

  // Id of the Queue used in communication with thunk
  HSA_QUEUEID queue_id_;

  // Indicates is queue is valid
  bool valid_;

  // Indicates if queue is inactive
  int32_t active_;

  // Cached value of HsaNodeProperties.HSA_CAPABILITY.DoorbellType
  int doorbell_type_;

  // Handle of agent, which queue is attached to
  GpuAgent* agent_;

  // Handle of scratch memory descriptor
  ScratchInfo queue_scratch_;

  core::HsaEventCallback errors_callback_;

  void* errors_data_;

  // Forbid copying and moving of this object
  DISALLOW_COPY_AND_ASSIGN(HwAqlCommandProcessor);
};
}  // namespace amd
#endif  // header guard
