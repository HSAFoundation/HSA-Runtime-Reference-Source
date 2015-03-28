#include "core/inc/amd_sdma_cmdwriter_kv.h"

#include <algorithm>
#include <cstring>

namespace amd {
// SDMA packet for CI device.
// Reference: http://people.freedesktop.org/~agd5f/dma_packets.txt

const unsigned int SDMA_OP_NOP = 0;
const unsigned int SDMA_OP_COPY = 1;
const unsigned int SDMA_OP_FENCE = 5;
const unsigned int SDMA_SUBOP_COPY_LINEAR = 0;

typedef struct SDMA_PKT_COPY_LINEAR_TAG {
  union {
    struct {
      unsigned int op : 8;
      unsigned int sub_op : 8;
      unsigned int extra_info : 16;
    };
    unsigned int DW_0_DATA;
  } HEADER_UNION;

  union {
    struct {
      unsigned int count : 22;
      unsigned int reserved_0 : 10;
    };
    unsigned int DW_1_DATA;
  } COUNT_UNION;

  union {
    struct {
      unsigned int reserved_0 : 16;
      unsigned int dst_swap : 2;
      unsigned int reserved_1 : 6;
      unsigned int src_swap : 2;
      unsigned int reserved_2 : 6;
    };
    unsigned int DW_2_DATA;
  } PARAMETER_UNION;

  union {
    struct {
      unsigned int src_addr_31_0 : 32;
    };
    unsigned int DW_3_DATA;
  } SRC_ADDR_LO_UNION;

  union {
    struct {
      unsigned int src_addr_63_32 : 32;
    };
    unsigned int DW_4_DATA;
  } SRC_ADDR_HI_UNION;

  union {
    struct {
      unsigned int dst_addr_31_0 : 32;
    };
    unsigned int DW_5_DATA;
  } DST_ADDR_LO_UNION;

  union {
    struct {
      unsigned int dst_addr_63_32 : 32;
    };
    unsigned int DW_6_DATA;
  } DST_ADDR_HI_UNION;
} SDMA_PKT_COPY_LINEAR;

typedef struct SDMA_PKT_FENCE_TAG {
  union {
    struct {
      unsigned int op : 8;
      unsigned int sub_op : 8;
      unsigned int reserved_0 : 16;
    };
    unsigned int DW_0_DATA;
  } HEADER_UNION;

  union {
    struct {
      unsigned int addr_31_0 : 32;
    };
    unsigned int DW_1_DATA;
  } ADDR_LO_UNION;

  union {
    struct {
      unsigned int addr_63_32 : 32;
    };
    unsigned int DW_2_DATA;
  } ADDR_HI_UNION;

  union {
    struct {
      unsigned int data : 32;
    };
    unsigned int DW_3_DATA;
  } DATA_UNION;
} SDMA_PKT_FENCE;

inline uint32_t ptrlow32(const void* p) {
  return static_cast<uint32_t>(reinterpret_cast<uintptr_t>(p));
}

inline uint32_t ptrhigh32(const void* p) {
#if defined(HSA_LARGE_MODEL)
  return static_cast<uint32_t>(reinterpret_cast<uintptr_t>(p) >> 32);
#else
  return 0;
#endif
}

SdmaCmdwriterKv::SdmaCmdwriterKv(size_t sdma_queue_buffer_size) {
  linear_copy_command_size_ = sizeof(SDMA_PKT_COPY_LINEAR);
  fence_command_size_ = sizeof(SDMA_PKT_FENCE);

  const uint32_t sync_command_size = fence_command_size_;
  const uint32_t max_num_copy_command = std::floor(
      (static_cast<uint32_t>(sdma_queue_buffer_size) - sync_command_size) /
      linear_copy_command_size_);

  max_single_linear_copy_size_ = 0x3fffe0;
  max_total_linear_copy_size_ = static_cast<size_t>(
      std::min(static_cast<uint64_t>(SIZE_MAX),
               static_cast<uint64_t>(max_num_copy_command) *
                   static_cast<uint64_t>(max_single_linear_copy_size_)));
}

SdmaCmdwriterKv::~SdmaCmdwriterKv() {}

void SdmaCmdwriterKv::WriteLinearCopyCommand(char* command_buffer, void* dst,
                                             const void* src,
                                             uint32_t copy_size) {
  SDMA_PKT_COPY_LINEAR* packet_addr =
      reinterpret_cast<SDMA_PKT_COPY_LINEAR*>(command_buffer);

  memset(packet_addr, 0, sizeof(SDMA_PKT_COPY_LINEAR));

  packet_addr->HEADER_UNION.op = SDMA_OP_COPY;
  packet_addr->HEADER_UNION.sub_op = SDMA_SUBOP_COPY_LINEAR;

  packet_addr->COUNT_UNION.count = copy_size;

  packet_addr->SRC_ADDR_LO_UNION.src_addr_31_0 = ptrlow32(src);
  packet_addr->SRC_ADDR_HI_UNION.src_addr_63_32 = ptrhigh32(src);

  packet_addr->DST_ADDR_LO_UNION.dst_addr_31_0 = ptrlow32(dst);
  packet_addr->DST_ADDR_HI_UNION.dst_addr_63_32 = ptrhigh32(dst);
}

void SdmaCmdwriterKv::WriteFenceCommand(char* command_buffer,
                                        uint32_t* fence_address,
                                        uint32_t fence_value) {
  SDMA_PKT_FENCE* packet_addr =
      reinterpret_cast<SDMA_PKT_FENCE*>(command_buffer);

  memset(packet_addr, 0, sizeof(SDMA_PKT_FENCE));

  packet_addr->HEADER_UNION.op = SDMA_OP_FENCE;

  packet_addr->ADDR_LO_UNION.addr_31_0 = ptrlow32(fence_address);

  packet_addr->ADDR_HI_UNION.addr_63_32 = ptrhigh32(fence_address);

  packet_addr->DATA_UNION.data = fence_value;
}
}  // namespace