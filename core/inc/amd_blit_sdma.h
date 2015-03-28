#ifndef HSA_RUNTIME_CORE_INC_AMD_BLIT_SDMA_H_
#define HSA_RUNTIME_CORE_INC_AMD_BLIT_SDMA_H_

#include <stdint.h>

#include "core/inc/amd_sdma_cmdwriter_kv.h"
#include "core/inc/blit.h"
#include "core/inc/runtime.h"
#include "core/inc/signal.h"

namespace amd {
class BlitSdma : public core::Blit {
 public:
  explicit BlitSdma();

  virtual ~BlitSdma() override;

  /// @brief Initialize a User Mode SDMA Queue object. Input parameters specify
  /// properties of queue being created.
  ///
  /// @param agent Pointer to the agent that will execute the PM4 commands.
  ///
  /// @return hsa_status_t
  virtual hsa_status_t Initialize(const core::Agent& agent) override;

  /// @brief Marks the queue object as invalid and uncouples its link with
  /// the underlying compute device's control block. Use of queue object
  /// once it has been release is illegal and any behavior is indeterminate
  ///
  /// @note: The call will block until all packets have executed.
  ///
  /// @return hsa_status_t
  virtual hsa_status_t Destroy() override;

  /// @brief Submit a linear copy command to the queue buffer.
  ///
  /// @param dst Memory address of the copy destination.
  /// @param src Memory address of the copy source.
  /// @param size Size of the data to be copied.
  virtual hsa_status_t SubmitLinearCopyCommand(void* dst, const void* src,
                                               size_t size) override;

 private:
  /// @brief Acquires the address into queue buffer where a new command
  /// packet of specified size could be written. The address that is
  /// returned is guaranteed to be unique even in a multi-threaded access
  /// scenario. This function is guaranteed to return a pointer for writing
  /// data into the queue buffer.
  ///
  /// @param cmd_size Command packet size in bytes.
  ///
  /// @return pointer into the queue buffer where a PM4 packet of specified size
  /// could be written. NULL if input size is greater than the size of queue
  /// buffer.
  char* AcquireWriteAddress(uint32_t cmd_size);

  /// @brief Updates the Write Register of compute device to the end of
  /// SDMA packet written into queue buffer. The update to Write Register
  /// will be safe under multi-threaded usage scenario. Furthermore, updates
  /// to Write Register are blocking until all prior updates are completed
  /// i.e. if two threads T1 & T2 were to call release, then updates by T2
  /// will block until T1 has completed its update (assumes T1 acquired the
  /// write address first).
  ///
  /// @param cmd_addr pointer into the queue buffer where a PM4 packet was
  /// written.
  ///
  /// @param cmd_size Command packet size in bytes.
  void ReleaseWriteAddress(char* cmd_addr, uint32_t cmd_size);

  /// @brief Writes NO-OP words into queue buffer in case writing a command
  /// causes the queue buffer to wrap.
  ///
  /// @param cmd_size Size in bytes of command causing queue buffer to wrap.
  void WrapQueue(uint32_t cmd_size);

  /// @brief Submit a fence command to the queue buffer and block until the
  ///        fence command is executed.
  void Fence(char* fence_command_addr);

  /// Indicates size of Queue buffer in bytes.
  uint32_t queue_size_;

  /// Base address of the Queue buffer at construction time.
  char* queue_start_addr_;

  /// End address of the Queue buffer at construction time.
  char* queue_end_addr_;

  /// Queue resource descriptor for doorbell, read
  /// and write indices
  HsaQueueResourceFixed queue_resource_;

  /// @brief Current address of execution in Queue buffer.
  ///
  /// @note: The value of address is obtained by reading
  /// the value of Write Register of the compute device.
  /// Users should write to the Queue buffer at the current
  /// address, else it will lead to execution error and potentially
  /// a hang.
  ///
  /// @note: The value of Write Register does not always begin
  /// with Zero after a Queue has been created. This needs to be
  /// understood better. This means that current address number of
  /// words of Queue buffer is unavailable for use.
  uint32_t cached_offset_;

  /// Device specific command writer.
  SdmaCmdwriterKv* cmdwriter_;
};
}  // namespace amd

#endif  // header guard
