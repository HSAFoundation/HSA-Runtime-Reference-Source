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

#ifndef HSA_RUNTME_CORE_INC_COMMAND_QUEUE_H_
#define HSA_RUNTME_CORE_INC_COMMAND_QUEUE_H_

#include "core/inc/runtime.h"
#include "core/inc/checked.h"
#include "core/util/utils.h"

#include <sstream>
#include "amd_hsa_queue.h"

namespace core {
struct AqlPacket {
  union {
    hsa_kernel_dispatch_packet_t dispatch;
    hsa_barrier_and_packet_t barrier_and;
    hsa_barrier_or_packet_t barrier_or;
    hsa_agent_dispatch_packet_t agent;
  };

  bool IsValid() {
    const uint8_t packet_type = dispatch.header >> HSA_PACKET_HEADER_TYPE;
    return (packet_type > HSA_PACKET_TYPE_INVALID &&
            packet_type <= HSA_PACKET_TYPE_BARRIER_OR);
  }

  std::string string() const {
    std::stringstream string;
    uint8_t type = ((dispatch.header >> HSA_PACKET_HEADER_TYPE) &
                    ((1 << HSA_PACKET_HEADER_WIDTH_TYPE) - 1));

    const char* type_names[] = {
        "HSA_PACKET_TYPE_VENDOR_SPECIFIC", "HSA_PACKET_TYPE_INVALID",
        "HSA_PACKET_TYPE_KERNEL_DISPATCH", "HSA_PACKET_TYPE_BARRIER_AND",
        "HSA_PACKET_TYPE_AGENT_DISPATCH",  "HSA_PACKET_TYPE_BARRIER_OR"};

    string << "type: " << type_names[type]
           << "\nbarrier: " << ((dispatch.header >> HSA_PACKET_HEADER_BARRIER) &
                                ((1 << HSA_PACKET_HEADER_WIDTH_BARRIER) - 1))
           << "\nacquire: "
           << ((dispatch.header >> HSA_PACKET_HEADER_ACQUIRE_FENCE_SCOPE) &
               ((1 << HSA_PACKET_HEADER_WIDTH_ACQUIRE_FENCE_SCOPE) - 1))
           << "\nrelease: "
           << ((dispatch.header >> HSA_PACKET_HEADER_RELEASE_FENCE_SCOPE) &
               ((1 << HSA_PACKET_HEADER_WIDTH_RELEASE_FENCE_SCOPE) - 1));

    if (type == HSA_PACKET_TYPE_KERNEL_DISPATCH) {
      string << "\nDim: " << dispatch.setup
             << "\nworkgroup_size: " << dispatch.workgroup_size_x << ", "
             << dispatch.workgroup_size_y << ", " << dispatch.workgroup_size_z
             << "\ngrid_size: " << dispatch.grid_size_x << ", "
             << dispatch.grid_size_y << ", " << dispatch.grid_size_z
             << "\nprivate_size: " << dispatch.private_segment_size
             << "\ngroup_size: " << dispatch.group_segment_size
             << "\nkernel_object: " << dispatch.kernel_object
             << "\nkern_arg: " << dispatch.kernarg_address
             << "\nsignal: " << dispatch.completion_signal.handle;
    }

    if ((type == HSA_PACKET_TYPE_BARRIER_AND) ||
        (type == HSA_PACKET_TYPE_BARRIER_OR)) {
      for (int i = 0; i < 5; i++)
        string << "\ndep[" << i << "]: " << barrier_and.dep_signal[i].handle;
      string << "\nsignal: " << barrier_and.completion_signal.handle;
    }

    return string.str();
  }
};

/// @brief Class Queue which encapsulate user mode queues and
/// provides Api to access its Read, Write indices using Acquire,
/// Release and Relaxed semantics.
/*
Queue is intended to be an pure interface class and may be wrapped or replaced
by tools.
All funtions other than Convert and public_handle must be virtual.
*/
class Queue : public Checked<0xFA3906A679F9DB49> {
 public:
  Queue() { public_handle_ = Convert(this); }
  virtual ~Queue() {}

  /// @brief Returns the handle of Queue's public data type
  ///
  /// @param queue Pointer to an instance of Queue implementation object
  ///
  /// @return hsa_queue_t * Pointer to the public data type of a queue
  static __forceinline hsa_queue_t* Convert(Queue* queue) {
    return &queue->amd_queue_.hsa_queue;
  }

  /// @brief Transform the public data type of a Queue's data type into an
  //  instance of it Queue class object
  ///
  /// @param queue Handle of public data type of a queue
  ///
  /// @return Queue * Pointer to the Queue's implementation object
  static __forceinline Queue* Convert(const hsa_queue_t* queue) {
    return (queue != NULL)
               ? reinterpret_cast<Queue*>(
                     reinterpret_cast<size_t>(queue) -
                     (reinterpret_cast<size_t>(&(reinterpret_cast<Queue*>(12345)
                                                     ->amd_queue_.hsa_queue)) -
                      12345))
               : NULL;
  }

  /// @brief Inactivate the queue object. Once inactivate a
  /// queue cannot be used anymore and must be destroyed
  ///
  /// @return hsa_status_t Status of request
  virtual hsa_status_t Inactivate() = 0;

  /// @brief Reads the Read Index of Queue using Acquire semantics
  ///
  /// @return uint64_t Value of Read index
  virtual uint64_t LoadReadIndexAcquire() = 0;

  /// @brief Reads the Read Index of Queue using Relaxed semantics
  ///
  /// @return uint64_t Value of Read index
  virtual uint64_t LoadReadIndexRelaxed() = 0;

  /// @brief Reads the Write Index of Queue using Acquire semantics
  ///
  /// @return uint64_t Value of Write index
  virtual uint64_t LoadWriteIndexAcquire() = 0;

  /// Reads the Write Index of Queue using Relaxed semantics
  ///
  /// @return uint64_t Value of Write index
  virtual uint64_t LoadWriteIndexRelaxed() = 0;

  /// @brief Updates the Read Index of Queue using Relaxed semantics
  ///
  /// @param value New value of Read index to update
  virtual void StoreReadIndexRelaxed(uint64_t value) = 0;

  /// @brief Updates the Read Index of Queue using Release semantics
  ///
  /// @param value New value of Read index to update
  virtual void StoreReadIndexRelease(uint64_t value) = 0;

  /// @brief Updates the Write Index of Queue using Relaxed semantics
  ///
  /// @param value New value of Write index to update
  virtual void StoreWriteIndexRelaxed(uint64_t value) = 0;

  /// @brief Updates the Write Index of Queue using Release semantics
  ///
  /// @param value New value of Write index to update
  virtual void StoreWriteIndexRelease(uint64_t value) = 0;

  /// @brief Compares and swaps Write index using Acquire and Release semantics
  ///
  /// @param expected Current value of write index
  ///
  /// @param value Value of new write index
  ///
  /// @return uint64_t Value of write index before the update
  virtual uint64_t CasWriteIndexAcqRel(uint64_t expected, uint64_t value) = 0;

  /// @brief Compares and swaps Write index using Acquire semantics
  ///
  /// @param expected Current value of write index
  ///
  /// @param value Value of new write index
  ///
  /// @return uint64_t Value of write index before the update
  virtual uint64_t CasWriteIndexAcquire(uint64_t expected, uint64_t value) = 0;

  /// @brief Compares and swaps Write index using Relaxed semantics
  ///
  /// @param expected Current value of write index
  ///
  /// @param value Value of new write index
  ///
  /// @return uint64_t Value of write index before the update
  virtual uint64_t CasWriteIndexRelaxed(uint64_t expected, uint64_t value) = 0;

  /// @brief Compares and swaps Write index using Release semantics
  ///
  /// @param expected Current value of write index
  ///
  /// @param value Value of new write index
  ///
  /// @return uint64_t Value of write index before the update
  virtual uint64_t CasWriteIndexRelease(uint64_t expected, uint64_t value) = 0;

  /// @brief Updates the Write index using Acquire and Release semantics
  ///
  /// @param value Value of new write index
  ///
  /// @return uint64_t Value of write index before the update
  virtual uint64_t AddWriteIndexAcqRel(uint64_t value) = 0;

  /// @brief Updates the Write index using Acquire semantics
  ///
  /// @param value Value of new write index
  ///
  /// @return uint64_t Value of write index before the update
  virtual uint64_t AddWriteIndexAcquire(uint64_t value) = 0;

  /// @brief Updates the Write index using Relaxed semantics
  ///
  /// @param value Value of new write index
  ///
  /// @return uint64_t Value of write index before the update
  virtual uint64_t AddWriteIndexRelaxed(uint64_t value) = 0;

  /// @brief Updates the Write index using Release semantics
  ///
  /// @param value Value of new write index
  ///
  /// @return uint64_t Value of write index before the update
  virtual uint64_t AddWriteIndexRelease(uint64_t value) = 0;

  /// @brief Set CU Masking
  ///
  /// @param num_cu_mask_count size of mask bit array
  ///
  /// @param cu_mask pointer to cu mask
  ///
  /// @return hsa_status_t
  virtual hsa_status_t SetCUMasking(const uint32_t num_cu_mask_count,
                                    const uint32_t* cu_mask) = 0;

  // Handle of Amd Queue struct
  amd_queue_t amd_queue_;

  hsa_queue_t* public_handle() const { return public_handle_; }

 protected:
  static void set_public_handle(Queue* ptr, hsa_queue_t* handle) {
    ptr->do_set_public_handle(handle);
  }
  virtual void do_set_public_handle(hsa_queue_t* handle) {
    public_handle_ = handle;
  }
  hsa_queue_t* public_handle_;

 private:
  DISALLOW_COPY_AND_ASSIGN(Queue);
};
}

#endif  // header guard
