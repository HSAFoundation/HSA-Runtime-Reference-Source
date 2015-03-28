#ifndef HSA_RUNTIME_CORE_AMD_SDMA_COMDWRITER_KV_H_
#define HSA_RUNTIME_CORE_AMD_SDMA_COMDWRITER_KV_H_

#include <stdint.h>

#include "core/inc/runtime.h"
#include "core/util/utils.h"

namespace amd {
class SdmaCmdwriterKv {
 public:
  explicit SdmaCmdwriterKv(size_t sdma_queue_buffer_size);

  virtual ~SdmaCmdwriterKv();

  virtual void WriteLinearCopyCommand(char* command_buffer, void* dst,
                                      const void* src, uint32_t copy_size);

  virtual void WriteFenceCommand(char* command_buffer, uint32_t* fence_address,
                                 uint32_t fence_value);

  inline uint32_t linear_copy_command_size() {
    return linear_copy_command_size_;
  }

  inline uint32_t fence_command_size() { return fence_command_size_; }

  inline size_t max_single_linear_copy_size() {
    return max_single_linear_copy_size_;
  }

  inline size_t max_total_linear_copy_size() {
    return max_total_linear_copy_size_;
  }

 protected:
  uint32_t linear_copy_command_size_;
  uint32_t fence_command_size_;

  /// Max copy size of a single linear copy command packet.
  size_t max_single_linear_copy_size_;

  /// Max total copy size supported by this queue.
  size_t max_total_linear_copy_size_;

  DISALLOW_COPY_AND_ASSIGN(SdmaCmdwriterKv);
};
}  // namespace
#endif
