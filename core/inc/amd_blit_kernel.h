#ifndef HSA_RUNTIME_CORE_INC_AMD_BLIT_KERNEL_H_
#define HSA_RUNTIME_CORE_INC_AMD_BLIT_KERNEL_H_

#include <stdint.h>

#include "core/inc/blit.h"

namespace amd {
class BlitKernel : public core::Blit {
 public:
  explicit BlitKernel();
  virtual ~BlitKernel() override;

  /// @brief Initialize a blit kernel object.
  ///
  /// @param agent Pointer to the agent that will execute the AQL packets.
  ///
  /// @return hsa_status_t
  virtual hsa_status_t Initialize(const core::Agent& agent) override;

  /// @brief Marks the blit kernel object as invalid and uncouples its link with
  /// the underlying AQL kernel queue. Use of the blit object
  /// once it has been release is illegal and any behavior is indeterminate
  ///
  /// @note: The call will block until all AQL packets have been executed.
  ///
  /// @return hsa_status_t
  virtual hsa_status_t Destroy() override;

  /// @brief Submit an AQL packet to perform vector copy. The call is blocking
  /// until the command execution is
  /// finished.
  ///
  /// @param dst Memory address of the copy destination.
  /// @param src Memory address of the copy source.
  /// @param size Size of the data to be copied.
  virtual hsa_status_t SubmitLinearCopyCommand(void* dst, const void* src,
                                               size_t size) override;

 private:
  /// Reserve a slot in the queue buffer. The call will wait until the queue
  /// buffer has a room.
  uint64_t AcquireWriteIndex();

  /// Update the queue doorbell register with ::write_index. This
  /// function also serializes concurrent doorbell update to ensure that the
  /// packet processor doesn't get invalid packet.
  void ReleaseWriteIndex(uint64_t write_index);

  /// Wait until all packets are finished.
  hsa_status_t Fence();

  /// Handles to the vector copy kernel.
  hsa_executable_t code_executable_;
  hsa_code_object_t code_object_;
  uint64_t code_handle_;
  uint32_t code_private_segment_size_;

  /// AQL queue for submitting the vector copy kernel.
  hsa_queue_t* queue_;

  /// Index to track concurrent kernel launch.
  volatile std::atomic<uint64_t> cached_index_;
};
}  // namespace amd

#endif  // header guard
