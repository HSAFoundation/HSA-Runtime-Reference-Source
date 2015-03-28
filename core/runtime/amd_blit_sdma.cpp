#include "core/inc/amd_blit_sdma.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstring>

#include "core/inc/amd_gpu_agent.h"
#include "core/inc/thunk.h"
#include "core/util/utils.h"

namespace amd {
/**
* Determines if adding cmd_size to queue buffer will lead write offset to
* exceed buffer size.
*
* @param start_addr Beginning address of queue buffer.
*
* @param end_addr End address of queue buffer.
*
* @param cmd_size Size of SDMA commands in bytes.
*/
inline bool CmdIsValid(char* start_addr, char* end_addr, uint32_t cmd_size) {
  return ((end_addr) > ((start_addr) + (cmd_size)));
}

BlitSdma::BlitSdma()
    : core::Blit(),
      queue_size_(0),
      queue_start_addr_(NULL),
      queue_end_addr_(NULL),
      cached_offset_(0),
      cmdwriter_(NULL) {
  std::memset(&queue_resource_, 0, sizeof(queue_resource_));
}

BlitSdma::~BlitSdma() {}

hsa_status_t BlitSdma::Initialize(const core::Agent& agent) {
  if (queue_start_addr_ != NULL && queue_size_ != 0) {
    // Already initialized.
    return HSA_STATUS_SUCCESS;
  }

  if (agent.device_type() != core::Agent::kAmdGpuDevice) {
    return HSA_STATUS_ERROR;
  }

  // Allocate queue buffer.
  const size_t kPageSize = 4096;
  const size_t kSdmaQueueSize = 1024 * 1024 * 2;

  queue_size_ = kSdmaQueueSize;
  queue_start_addr_ =
      reinterpret_cast<char*>(_aligned_malloc(queue_size_, kPageSize));
  queue_end_addr_ = queue_start_addr_ + queue_size_;
  std::memset(queue_start_addr_, 0, queue_size_);

  hsa_status_t status = HSA::hsa_memory_register(queue_start_addr_, queue_size_);
  if (status != HSA_STATUS_SUCCESS) {
    Destroy();
    return status;
  }

  // Access kernel driver to initialize the queue control block
  // This call binds user mode queue object to underlying compute
  // device.
  const GpuAgent& gpu_agent = reinterpret_cast<const GpuAgent&>(agent);
  const HSA_QUEUE_TYPE kQueueType_ = HSA_QUEUE_SDMA;
  if (HSAKMT_STATUS_SUCCESS !=
      hsaKmtCreateQueue(gpu_agent.node_id(), kQueueType_, 100,
                        HSA_QUEUE_PRIORITY_MAXIMUM, queue_start_addr_,
                        queue_size_, NULL, &queue_resource_)) {
    Destroy();
    return HSA_STATUS_ERROR_OUT_OF_RESOURCES;
  }

  cached_offset_ = *(queue_resource_.queue_writeptr);

  // Currently only support KV.
  cmdwriter_ = new SdmaCmdwriterKv(queue_size_);

  return HSA_STATUS_SUCCESS;
}

hsa_status_t BlitSdma::Destroy(void) {
  // Release all allocated resources and reset them to zero.

  if (cmdwriter_ != NULL) {
    delete cmdwriter_;
    cmdwriter_ = NULL;

    // Release queue resources from the kernel
    hsaKmtDestroyQueue(queue_resource_.queue_id);
    memset(&queue_resource_, 0, sizeof(queue_resource_));
  }

  if (queue_start_addr_ != NULL && queue_size_ != 0) {
    // Deregister and release queue buffer.
    HSA::hsa_memory_deregister(queue_start_addr_, queue_size_);
    _aligned_free(queue_start_addr_);
  }

  queue_size_ = 0;
  queue_start_addr_ = NULL;
  queue_end_addr_ = NULL;
  cached_offset_ = 0;

  return HSA_STATUS_SUCCESS;
}

hsa_status_t BlitSdma::SubmitLinearCopyCommand(void* dst, const void* src,
                                                size_t size) {
  assert(cmdwriter_ != NULL);

  if (size > cmdwriter_->max_total_linear_copy_size()) {
    return HSA_STATUS_ERROR_OUT_OF_RESOURCES;
  }

  // Break the copy into multiple copy operation incase the copy size exceeds
  // the SDMA linear copy limit.
  const uint32_t num_copy_command = static_cast<uint32_t>(std::ceil(
      static_cast<double>(size) / cmdwriter_->max_single_linear_copy_size()));

  const uint32_t total_copy_command_size =
      num_copy_command * cmdwriter_->linear_copy_command_size();

  const uint32_t total_command_size =
      total_copy_command_size + cmdwriter_->fence_command_size();

  char* command_addr = AcquireWriteAddress(total_command_size);

  if (command_addr == NULL) {
    return HSA_STATUS_ERROR_OUT_OF_RESOURCES;
  }

  const uint32_t copy_command_size = cmdwriter_->linear_copy_command_size();
  size_t cur_size = 0;
  for (uint32_t i = 0; i < num_copy_command; ++i) {
    const uint32_t copy_size = static_cast<uint32_t>(
        std::min((size - cur_size), cmdwriter_->max_single_linear_copy_size()));

    void* cur_dst = static_cast<char*>(dst) + cur_size;
    const void* cur_src = static_cast<const char*>(src) + cur_size;

    cmdwriter_->WriteLinearCopyCommand(command_addr, cur_dst, cur_src,
                                       copy_size);

    ReleaseWriteAddress(command_addr, copy_command_size);

    command_addr += copy_command_size;
    cur_size += copy_size;
  }

  assert(cur_size == size);

  Fence(command_addr);

  return HSA_STATUS_SUCCESS;
}

char* BlitSdma::AcquireWriteAddress(uint32_t cmd_size) {
  assert(CmdIsValid(queue_start_addr_, queue_end_addr_, cmd_size));

  while (true) {
    uint32_t curr_offset = cached_offset_;
    const uint32_t end_offset = curr_offset + cmd_size;
    const char* cmd_end_addr = queue_start_addr_ + end_offset;

    if (cmd_end_addr >= queue_end_addr_) {
      // Queue buffer is not enough to contain the new command.

      // The safe space for the new command is the start of the queue buffer to
      // the last read address.
      if (*queue_resource_.queue_readptr < cmd_size) {
        // There is no safe space to use currently.
        return NULL;
      }

      WrapQueue(cmd_size);

      continue;
    }

    if (std::atomic_compare_exchange_weak(
            reinterpret_cast<std::atomic<uint32_t>*>(&cached_offset_),
            &curr_offset, end_offset)) {
      return queue_start_addr_ + curr_offset;
    }
  }

  return NULL;
}

static void UpdateWriteAndDoorbellRegister(
    HsaQueueResourceFixed* queue_resource, uint32_t current_offset,
    uint32_t new_offset) {
  while (true) {
    // Make sure that the address before ::current_offset is already released.
    // Otherwise the CP may read invalid packets.
    uint32_t expected_offset = current_offset;
    if (std::atomic_compare_exchange_weak(
            reinterpret_cast<volatile std::atomic<uint32_t>*>(
                queue_resource->queue_writeptr),
            &expected_offset, new_offset)) {
      // Update doorbel register.
      *queue_resource->queue_doorbell = new_offset;
      break;
    }
  }
}

void BlitSdma::ReleaseWriteAddress(char* cmd_addr, uint32_t cmd_size) {
  assert(cmd_addr != NULL);
  assert(cmd_addr < queue_end_addr_);
  assert(cmd_addr >= queue_start_addr_);
  assert(CmdIsValid(queue_start_addr_, queue_end_addr_, cmd_size));

  // Update write register.
  const uint32_t curent_offset = cmd_addr - queue_start_addr_;
  const uint32_t new_offset = curent_offset + cmd_size;
  UpdateWriteAndDoorbellRegister(&queue_resource_, curent_offset, new_offset);
}

void BlitSdma::WrapQueue(uint32_t cmd_size) {
  // Re-determine the offset into queue buffer where NOOP instructions
  // should be written.
  while (true) {
    uint32_t curent_offset = cached_offset_;
    const uint32_t end_offset = curent_offset + cmd_size;
    const char* cmd_end_addr = queue_start_addr_ + end_offset;

    if (cmd_end_addr < queue_end_addr_) {
      return;
    }

    if (std::atomic_compare_exchange_weak(
            reinterpret_cast<std::atomic<uint32_t>*>(&cached_offset_),
            &curent_offset, 0U)) {
      // Fill the queue with NOOP commands.
      char* noop_address = queue_start_addr_ + curent_offset;
      const size_t noop_commands_size = queue_size_ - curent_offset;
      std::memset(noop_address, 0, noop_commands_size);

      // Update write and doorbell registers to execute NOOP instructions.
      UpdateWriteAndDoorbellRegister(&queue_resource_, curent_offset, 0);
    }
  }
}

void BlitSdma::Fence(char* fence_command_addr) {
  assert(fence_command_addr != NULL);
  const uint32_t fence_command_size = cmdwriter_->fence_command_size();

  const uint32_t kFenceValue = 2014;
  uint32_t fence = 0;
  HSA::hsa_memory_register(&fence, sizeof(fence));

  cmdwriter_->WriteFenceCommand(fence_command_addr, &fence, kFenceValue);

  ReleaseWriteAddress(fence_command_addr, fence_command_size);

  volatile uint32_t* fence_addr = reinterpret_cast<volatile uint32_t*>(&fence);
  int spin_count = 51;
  while (*fence_addr != kFenceValue) {
    if (--spin_count > 0) {
      continue;
    }
    os::YieldThread();
  }

  HSA::hsa_memory_deregister(&fence, sizeof(fence));
}

}  // namespace amd