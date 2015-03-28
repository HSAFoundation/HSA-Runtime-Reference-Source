#include "core/inc/amd_blit_kernel.h"

#include <algorithm>
#include <atomic>
#include <climits>
#include <cmath>
#include <cstring>

#include "core/inc/amd_blit_kernel_kv.h"
#include "core/inc/amd_gpu_agent.h"
#include "core/inc/hsa_internal.h"
#include "core/inc/thunk.h"
#include "core/util/utils.h"

namespace amd {

BlitKernel::BlitKernel()
    : core::Blit(),
      code_handle_(0),
      code_private_segment_size_(0),
      queue_(NULL),
      cached_index_(0) {}

BlitKernel::~BlitKernel() {}

hsa_status_t BlitKernel::Initialize(const core::Agent& agent) {
  hsa_agent_t agent_handle = agent.public_handle();

  uint32_t features = 0;
  hsa_status_t status =
      HSA::hsa_agent_get_info(agent_handle, HSA_AGENT_INFO_FEATURE, &features);
  if (status != HSA_STATUS_SUCCESS) {
    return status;
  }

  if ((features & HSA_AGENT_FEATURE_KERNEL_DISPATCH) == 0) {
    return HSA_STATUS_ERROR;
  }

  uint32_t max_queue_size = 0;
  status = HSA::hsa_agent_get_info(agent_handle, HSA_AGENT_INFO_QUEUE_MAX_SIZE,
                                   &max_queue_size);

  if (HSA_STATUS_SUCCESS != status) {
    return status;
  }

  status =
      HSA::hsa_queue_create(agent_handle, max_queue_size, HSA_QUEUE_TYPE_MULTI,
                            NULL, NULL, UINT_MAX, UINT_MAX, &queue_);

  if (HSA_STATUS_SUCCESS != status) {
    return status;
  }

  cached_index_ = 0;

  // TODO only support KV.
  void* raw_obj_mem = kVectorCopyKvObject;
  size_t raw_obj_size = kVectorCopyKvObjectSize;
  code_handle_ = 0;
  status = HSA::hsa_code_object_deserialize(raw_obj_mem, raw_obj_size, "",
                                            &code_object_);
  if (HSA_STATUS_SUCCESS != status) {
    return status;
  }

  // Create executable.
  status = HSA::hsa_executable_create(
      HSA_PROFILE_FULL, HSA_EXECUTABLE_STATE_UNFROZEN, "", &code_executable_);
  if (HSA_STATUS_SUCCESS != status) {
    return status;
  }

  // Load code object.
  status = HSA::hsa_executable_load_code_object(code_executable_, agent_handle,
                                                code_object_, "");
  if (HSA_STATUS_SUCCESS != status) {
    return status;
  }

  // Freeze executable.
  status = HSA::hsa_executable_freeze(code_executable_, "");
  if (HSA_STATUS_SUCCESS != status) {
    return status;
  }

  // Get symbol handle.
  hsa_executable_symbol_t kernel_symbol;
  status = HSA::hsa_executable_get_symbol(code_executable_, "",
                                          "am::&__vector_copy_kernel",
                                          agent_handle, 0, &kernel_symbol);
  if (HSA_STATUS_SUCCESS != status) {
    return status;
  }

  // Get code handle.
  status = HSA::hsa_executable_symbol_get_info(
      kernel_symbol, HSA_EXECUTABLE_SYMBOL_INFO_KERNEL_OBJECT, &code_handle_);
  if (HSA_STATUS_SUCCESS != status) {
    return status;
  }

  // Get private segment size.
  status = HSA::hsa_executable_symbol_get_info(
      kernel_symbol, HSA_EXECUTABLE_SYMBOL_INFO_KERNEL_PRIVATE_SEGMENT_SIZE,
      &code_private_segment_size_);
  if (HSA_STATUS_SUCCESS != status) {
    return status;
  }

  return HSA_STATUS_SUCCESS;
}

hsa_status_t BlitKernel::Destroy(void) {
  Fence();

  if (queue_ != NULL) {
    HSA::hsa_queue_destroy(queue_);
  }

  HSA::hsa_executable_destroy(code_executable_);

  HSA::hsa_code_object_destroy(code_object_);

  return HSA_STATUS_SUCCESS;
}

hsa_status_t BlitKernel::SubmitLinearCopyCommand(void* dst, const void* src,
                                                 size_t size) {
  assert(code_handle_ != 0);

  struct __ALIGNED__(16) KernelArgs {
    const void* src_;
    void* dst_;
    uint64_t size_;
  };

  const uint32_t bitmask = queue_->size - 1;
  size_t total_copy_size = 0;
  while (total_copy_size < size) {
    hsa_kernel_dispatch_packet_t packet = {0};

    static const uint16_t kDispatchPacketHeader =
        (HSA_PACKET_TYPE_KERNEL_DISPATCH << HSA_PACKET_HEADER_TYPE) |
        (1 << HSA_PACKET_HEADER_BARRIER) |
        (HSA_FENCE_SCOPE_AGENT << HSA_PACKET_HEADER_ACQUIRE_FENCE_SCOPE) |
        (HSA_FENCE_SCOPE_AGENT << HSA_PACKET_HEADER_RELEASE_FENCE_SCOPE);

    packet.header = kDispatchPacketHeader;

    packet.kernel_object = code_handle_;
    packet.private_segment_size = code_private_segment_size_;

    // Setup arguments.
    static const size_t kMaxCopySize = UINT32_MAX;
    const uint32_t copy_size =
        static_cast<uint32_t>(std::min((size - total_copy_size), kMaxCopySize));

    void* cur_dst = static_cast<char*>(dst) + total_copy_size;
    const void* cur_src = static_cast<const char*>(src) + total_copy_size;

    KernelArgs args = {0};

    args.src_ = cur_src;
    args.dst_ = cur_dst;
    args.size_ = copy_size;

    packet.kernarg_address = &args;

    // Setup working size.
    const int kNumDimension = 1;
    packet.setup = kNumDimension << HSA_KERNEL_DISPATCH_PACKET_SETUP_DIMENSIONS;
    packet.grid_size_x = AlignUp(static_cast<uint32_t>(copy_size), 256);
    packet.grid_size_y = packet.grid_size_z = 1;
    packet.workgroup_size_x = 256;
    packet.workgroup_size_y = packet.workgroup_size_z = 1;

    // Reserve write index.
    uint64_t write_index = AcquireWriteIndex();

    // Populate queue buffer with AQL packet.
    hsa_kernel_dispatch_packet_t* queue_buffer =
        reinterpret_cast<hsa_kernel_dispatch_packet_t*>(queue_->base_address);
    queue_buffer[write_index & bitmask] = packet;

    // Launch packet.
    ReleaseWriteIndex(write_index);

    total_copy_size += copy_size;
  }

  return Fence();
}

uint64_t BlitKernel::AcquireWriteIndex() {
  uint64_t write_index = HSA::hsa_queue_add_write_index_acq_rel(queue_, 1);

  while (true) {
    // Wait until we have room in the queue;
    const uint64_t read_index = HSA::hsa_queue_load_read_index_relaxed(queue_);
    if ((write_index - read_index) < queue_->size) {
      break;
    }
  }

  return write_index;
}

void BlitKernel::ReleaseWriteIndex(uint64_t write_index) {
  // Launch packet.
  while (true) {
    // Make sure that the address before ::current_offset is already released.
    // Otherwise the packet processor may read invalid packets.
    uint64_t expected_offset = write_index;
    if (std::atomic_compare_exchange_weak_explicit(
            &cached_index_, &expected_offset, write_index + 1,
            std::memory_order_relaxed, std::memory_order_relaxed)) {
      // Update doorbel register.
      HSA::hsa_signal_store_release(queue_->doorbell_signal, write_index);
      break;
    }
  }
}

hsa_status_t BlitKernel::Fence() {
  const uint16_t kBarrierPacketHeader =
      (HSA_PACKET_TYPE_BARRIER_AND << HSA_PACKET_HEADER_TYPE) |
      (1 << HSA_PACKET_HEADER_BARRIER) |
      (HSA_FENCE_SCOPE_NONE << HSA_PACKET_HEADER_ACQUIRE_FENCE_SCOPE) |
      (HSA_FENCE_SCOPE_SYSTEM << HSA_PACKET_HEADER_RELEASE_FENCE_SCOPE);

  hsa_barrier_and_packet_t packet = {0};
  packet.header = kBarrierPacketHeader;

  hsa_signal_t kernel_signal = {0};

  hsa_status_t status = HSA::hsa_signal_create(1, 0, NULL, &kernel_signal);
  if (HSA_STATUS_SUCCESS != status) {
    return status;
  }
  packet.completion_signal = kernel_signal;

  const uint32_t bitmask = queue_->size - 1;

  // Reserve write index.
  uint64_t write_index = AcquireWriteIndex();

  // Populate queue buffer with AQL packet.
  hsa_barrier_and_packet_t* queue_buffer =
      reinterpret_cast<hsa_barrier_and_packet_t*>(queue_->base_address);
  queue_buffer[write_index & bitmask] = packet;

  // Launch packet.
  ReleaseWriteIndex(write_index);

  // Wait for the packet to finish.
  if (HSA::hsa_signal_wait_acquire(packet.completion_signal,
                                   HSA_SIGNAL_CONDITION_LT, 1, uint64_t(-1),
                                   HSA_WAIT_STATE_ACTIVE) != 0) {
    // Signal wait returned unexpected value.
    return HSA_STATUS_ERROR;
  }

  // Cleanup
  status = HSA::hsa_signal_destroy(packet.completion_signal);
  assert(status == HSA_STATUS_SUCCESS);

  return HSA_STATUS_SUCCESS;
}

}  // namespace amd