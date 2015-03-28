#ifndef HSA_RUNTIME_CORE_INC_BLIT_H_
#define HSA_RUNTIME_CORE_INC_BLIT_H_

#include <stdint.h>

#include "core/inc/agent.h"

namespace core {
class Blit {
 public:
  explicit Blit() {}
  virtual ~Blit() {}

  /// @brief Initialize a blit object.
  ///
  /// @param agent Pointer to the agent that will execute the blit commands.
  ///
  /// @return hsa_status_t
  virtual hsa_status_t Initialize(const core::Agent& agent) = 0;

  /// @brief Marks the blit object as invalid and uncouples its link with
  /// the underlying compute device's control block. Use of blit object
  /// once it has been release is illegal and any behavior is indeterminate
  ///
  /// @note: The call will block until all commands have executed.
  ///
  /// @return hsa_status_t
  virtual hsa_status_t Destroy() = 0;

  /// @brief Submit a linear copy command to the the underlying compute device's
  /// control block. The call is blocking until the command execution is
  /// finished.
  ///
  /// @param dst Memory address of the copy destination.
  /// @param src Memory address of the copy source.
  /// @param size Size of the data to be copied.
  virtual hsa_status_t SubmitLinearCopyCommand(void* dst, const void* src,
                                               size_t size) = 0;
};
}  // namespace core

#endif  // header guard
