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

// This file is used only for open source cmake builds.

#ifndef HSA_RUNTME_CORE_INC_SC_INTERFACE_H_
#define HSA_RUNTME_CORE_INC_SC_INTERFACE_H_

#include "stdint.h"

#include "inc/hsa_ext_finalize.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__GNUC__)
# define __ALIGNED__(x) __attribute__((aligned(x)))
#elif defined(_MSC_VER)
# define __ALIGNED__(x) __declspec(align(x))
#elif defined(RC_INVOKED)
# define __ALIGNED__(x)
#else
# error
#endif

/// The device specific data is accessible to kernel code and is used
/// to support HSAIL operations such as stof, maxcuid and maxwaveid.
///
/// Must be allocated on 64 byte alignment.
///
/// amd_queue_t object cannot span a 4GiB boundary. This allows CP
/// to save a few instructions when calculating the base address
/// of amd_queue_t from &(amd_queue_t.read_dispatch_id) and
/// amd_queue_t.read_dispatch_id_field_base_offset.
///
/// Note: there is a specific requirement for a Finalizer to convert
/// dword offsets to packet-granularity for
/// write_dispatch_id/read_dispatch_id accesses on Kaveri systems.
#define AMD_QUEUE_ALIGN_BYTES    64
#define AMD_QUEUE_ALIGN __ALIGNED__(AMD_QUEUE_ALIGN_BYTES)
typedef AMD_QUEUE_ALIGN struct amd_queue_s {

  /// HSA Architectured Queue object.
  hsa_queue_t hsa_queue;

  /// Unused. Allows hsa_queue_t to expand but still keeps
  /// write_dispatch_id, which is written by the producer (often the
  /// host CPU), in the same cache line. The following fields include
  /// read_dispatch_id, which is written by packet processor and so
  /// needs to be in a different cache line to write_dispatch_id to
  /// avoid cache line thrashing. Must be 0.
  uint32_t reserved1[4];

  /// A 64-bit unsigned integer which specifies the Dispatch ID of the
  /// next AQL packet to be allocated by the application or user-level
  /// runtime. Base + ((write_dispatch_id % size_packets) * AQL packet
  /// size) is the virtual address for the next AQL packet
  /// allocated. Initialized to 0 at queue creation time.
  ///
  /// Note: On GFX7, the write_dispatch_id is specified in dwords and
  /// must be divided by (AQL packet_size of 64 / dword size of 4)=16
  /// to obtain the HSA "packet-granularity" write offset. On GFX7,
  /// Base + (((write_dispatch_id/(AQL packet size / sizeof(dword))) %
  /// size_packets) * AQL packet size) is the virtual address for the
  /// next AQL packet allocated. Finalizer and Runtime software is
  /// responsible for converting dword offsets to packet granularity.
  volatile uint64_t write_dispatch_id;

  /// Start of cache line for fields accessed by Finalizer generated isa.

  /// For HSA64, the most significant 32 bits of the 64 bit group
  /// segment flat address aperture base. This is the same value as
  /// {SH_MEM_BASES:PRIVATE_BASE[15:13], 29'b0}.
  ///
  /// For HSA32, the 32 bits of the 32 bit group segment flat address
  /// aperture. This is the same value as
  /// {SH_MEM_BASES:SHARED_BASE[15:0], 16'b0}.
  ///
  /// Used in ISA to implement stof_group.
  ///
  uint32_t group_segment_aperture_base_hi;

  /// For HSA64, the most significant 32 bits of the 64 bit private
  /// segment flat address aperture base. This is the same value as
  /// {SH_MEM_BASES:PRIVATE_BASE[15:13], 28'b0, 1'b1}
  ///
  /// For HSA32, the 32 bits of the 32 bit private segment flat
  /// address aperture base. This is the same value as
  /// {SH_MEM_BASES:PRIVATE_BASE[15:0], 16'b0}.
  ///
  /// Used in ISA to implement stof_private.
  ///
  /// See description in Graphics IP v8 Address Space and Coherence
  /// Domains (GFXIP8).
  uint32_t private_segment_aperture_base_hi;

  /// The number of compute units - 1 on the device to which the queue
  /// is associated. Used in ISA to implement HSAIL maxcuid operation.
  uint32_t max_cu_id;

  /// The number of wavefronts - 1 that can be executed on a single
  /// compute unit of the device to which the queue is
  /// associated. Used in ISA to implement HSAIL maxwaveid operation.
  uint32_t max_wave_id;

  /// For queues attached to pre GFX9 hardware, it is necessary to
  /// prevent backwards doorbells in the software signal
  /// operation. This field is used to hold the maximum doorbell
  /// dispatch id [plus 1] signalled for the queue. The hardware will
  /// monitor this field, and not write_dispatch_id, on queue
  /// connect. The value written to this field is required to be made
  /// visible before writing to the queue's doorbell signal
  /// (referenced by hsa_queue.doorbell_signal) hardware location
  /// (referenced by hardware_doorbell_ptr or
  /// legacy_hardware_doorbell_ptr). The value is always 64 bit, even
  /// in small machine model. This field is not used by queues
  /// attached to GFX9 or later hardware. Must be initialized to 0.
  volatile uint64_t max_legacy_doorbell_dispatch_id_plus_1;

  /// For queues attached to pre GFX9 hardware, it is necessary to use
  /// a critical section to update the doorbell related fields of
  /// amd_queue_s.max_legacy_doorbell_dispatch_id_plus_1 and
  /// amd_signal_s.legacy_hardware_doorbell_ptr. This field is
  /// initialized to 0, and set to 1 to lock the critical
  /// section. This field is not used by queues attached to GFX9 or
  /// later hardware.
  volatile uint32_t legacy_doorbell_lock;

  /// Unused. Must be 0. If additional space is required for the
  /// fields accessed by the Finalizer generated isa, then this
  /// reserved space can be used, and space for an additional cache
  /// line(s) can also be added and not break backwards compatibility
  /// for CP micro code as the read_dispatch_id_field_base_offset
  /// field below can be initialized with the correct offset to get to
  /// the read_dispatch_id field below. If the CP micro code fields
  /// need to be expanded that is possible as there are no fields
  /// after them. This allows both the Fianlizer generated isa fields
  /// and CP micro code accessed fields to edpand in a backwards
  /// compatible way.
  uint32_t reserved2[9];

  /// Start of cache line for fields accessed by the packet processor
  /// (CP micro code). These are kept in a single cache line to
  /// minimize memory accesses performed by CP micro code.

  /// A 64-bit unsigned integer which specifies the Dispatch ID of the
  /// next AQL packet to be consumed by the compute unit hardware.
  /// Base + ((read_dispatch_id % size_packets) * AQL packet size) is
  /// the virtual address of the eldest AQL packet that is not yet
  /// released by the HSA Packet Processor. Initialized to 0 at queue
  /// creation time.
  ///
  /// Note: On GFX7, the read_dispatch_id is specified in dwords and
  /// must be divided by (AQL packet_size of 64 / dword size of 4)=16
  /// to obtain the HSA "packet-granularity" read offset. On GFX7,
  /// Base + (((read_dispatch_id/(AQL packet size / sizeof(dword))) %
  /// size_packets) * AQL packet size) is the virtual address of the
  /// eldest AQL packet that is not yet released by the HSA Packet
  /// Processor. Finalizer and Runtime software is responsible for
  /// converting dword offsets to packet granularity.
  volatile uint64_t read_dispatch_id;

  /// The byte offset from the base of hsa_queue_t to the
  /// read_dispatch_id field. The CP microcode uses this and
  /// CP_HQD_PQ_RPTR_REPORT_ADDR[_HI] to calculate the base address of
  /// hsa_queue_t when amd_kernel_code_t.enable_sgpr_dispatch_ptr is
  /// set. This field must immediately follow read_dispatch_id.
  ///
  /// This allows the layout above the read_dispatch_id field to change,
  /// and still be able to get the base of the hsa_queue_t, which is
  /// needed to return if amd_kernel_code_t.enable_sgpr_queue_ptr
  /// is requested. These fields are defined by HSA Foundation and so
  /// could change. CP only uses fields below read_dispatch_id which
  /// are defined by AMD.
  uint32_t read_dispatch_id_field_base_byte_offset;

  /// This is used by CP to set the COMPUTE_TMPRING_SIZE configuration
  /// register which defines how SPI allocates scratch memory to
  /// waves. The value must be consistent with the
  /// scratch_backing_memory_byte_size and scratch_workitem_byte_size
  /// fields. COMPUTE_TMPRING_SIZE.WAVES is the number of waves that
  /// can be launched given the values of
  /// scratch_backing_memory_byte_size and
  /// scratch_workitem_byte_size. COMPUTE_TMPRING_SIZE.WAVESIZE is
  /// amount of scatch memory used by each wave and must match
  /// scratch_workitem_byte_size.
  ///
  /// For GDX7, GFX8, GFX9: WAVES occupies bits 11:0 of
  /// compute_tmpring_size and WAVESIZE bits 24:12. For full
  /// occupancy, WAVES must be scratch waves per CU * total number of
  /// CUs on the chip. (scratch waves per CU is always 32 because SPI
  /// only has 32 slots to track per wave scratch allocations.)
  /// WAVESIZE is in units of 256 dwords (1024 bytes). WAVESIZE =
  /// ((scratch_workitem_byte_size * wavefront size) + ((256 * 4) -
  /// 1)) / (256 * 4)). On GFX8 this gives an effective per work-item
  /// scratch size of 0..4MiB-(256*4).
  uint32_t compute_tmpring_size;

  /// The following scratch_* fields manage the scratch
  /// memory. Scratch memory is used for the HSAIL private, spill and
  /// arg segments for a kernel executed by an AQL dispatch packets on
  /// the queue.

  /// An SRD that can be used to access the scratch memory for this
  /// queue. The base address will be the base of the scratch backing
  /// memory allocated to the queue and must be 256 byte aligned. On
  /// GFX7 (CI)/GFX8 (VI) this will be the same as
  /// SH_HIDDEN_PRIVATE_BASE_VMID +
  /// scratch_backing_memory_queue_location; on GFX9 (AI) and above
  /// this will be scratch_backing_memory_queue_location. The size
  /// will be scratch_workitem_byte_size * wavefront size (64 for GFX6
  /// (SI)/GFX7 (CI)/GFX8 (VI)/GFX9 (AI)) bytes. Swizzle enable will
  /// be set with an index stride of the wavefront size and element
  /// size (currently only 4 bytes is used). Tid enable will be
  /// set. ATC mode will be set to ATC or GPUVM as determined by
  /// Runtime. Mtype will be NC-NV (non-coherent/non-volatile). These
  /// values must match the flat scratch configuration values in
  /// SH_STATIC_MEM_CONFIG and SH_MEM_CONFIG. This is the value CP use
  /// to initalize the user SGPR when
  /// enable_sgpr_private_segment_buffer is specified in the
  /// amd_kernel_code_s.
  uint32_t scratch_resource_descriptor[4];

  /// This is the location of the scratch backing memory for this
  /// queue. Must be a 256 byte multiple.
  ///
  /// For GFX7 (CI)/GFX8 (VI) this is the byte offset from
  /// SH_HIDDEN_PRIVATE_BASE_VMID which is the base address of scratch
  /// memory for all queues in this process. The top 32 bits must be
  /// 0. If enable_sgpr_flat_scratch_init is specified in the
  /// amd_kernel_code_s then the lower 32 bits are used for the first
  /// user SGPR of the flat scratch init register pair by CP.
  ///
  /// For GFX9 (AI) and above this is a 64 bit address of this queues
  /// scratch backing memory. If enable_sgpr_flat_scratch_init is
  /// specified in the amd_kernel_code_s then all 64 bits are used for
  /// the user SGPR of the flat scratch init register pair by CP.
  uint64_t scratch_backing_memory_location;

  /// The total byte size of the scratch backing memory allocated for
  /// this queue. Must be a multiple of 256 dwords (1024 bytes). Used
  /// by the HSA Runtime when queue is destroyed to return the memory
  /// allocation. Will be set to a value that allows full occupancy:
  /// scratch_workitem_byte_size * wavefront size * scratch waves per
  /// CU (currently always 32) * number of CUs.
  ///
  /// This is must be consistent with compute_tmpring_size.
  uint64_t scratch_backing_memory_byte_size;
  
  /// The per-work item scratch size in bytes. Must be a multiple of
  /// the scratch swizzle element size (currently always 4). All
  /// dispatches that execute on a queue use the same per work-item
  /// scratch size. This allows all dispatches to share the same
  /// scratch backing memory, and allows different dispatches to
  /// execute concurrently. This also avoids having to change the SPI
  /// scratch settings between dispatches.
  ///
  /// CP uses this if it checks that the private segment size
  /// specified in a dispatch packet exceeds this value and reports a
  /// queue error. CP also uses this value to set the SPI scratch slot
  /// size when the queue is attached.
  ///
  /// On GFX7 (CI)/GFX8 (VI) if enable_sgpr_flat_scratch_init is
  /// specified in the amd_kernel_code_s then the 32 bits are used for
  /// the second user SGPR of the flat scratch init register pair by
  /// CP.
  ///
  /// On GFX9 (AI) onwards if enable_sgpr_private_segment_size is
  /// specified in the amd_kernel_code_s then the 32 bits are used for
  /// the user SGPR that holds the per work-item private segment size
  /// by CP.
  uint32_t scratch_workitem_byte_size;

  /// Runtime requires a trap handler to be enabled for all future
  /// kernel dispatches in this queue. This can be used by the runtime
  /// to ensure a trap handler is enabled to support a debugger.
  uint32_t enable_trap_handler :1;

  /// Are global memory addresses 64 bits. Must match
  /// hsa_compilationunit_code_t.hsail_machine_model ==
  /// HSA_MACHINE_LARGE. Must also match SH_MEM_CONFIG.PTR32 (GFX6
  /// (SI)/GFX7 (CI)), SH_MEM_CONFIG.ADDRESS_MODE (GFX8 (VI)+).
  uint32_t is_ptr64 :1;

  /// Trap handler requires trap SGPRs to be initialized with debug
  /// values for all future kernel dispatches in this queue. This can
  /// be used by the runtime to ensure a debugger trap handler will
  /// receive information needed to support a debugger. Used to set
  /// SPI_GDBG_TRAP_CONFIG.trap_en.
  uint32_t enable_trap_handler_debug_sgprs :1;

  /// Enable recording of timestamp information for profiling packet
  /// execution.
  uint32_t enable_profiling :1;

  /// Reserved. Must be 0.
  uint32_t reserved3 :28;

  /// Pad to next cache line. Reserved. Must be 0.
  uint32_t reserved4[2];

  /// Start of next cache line for fields not accessed under normal
  /// conditions by the packet processor (CP micro code). These are
  /// kept in a single cache line to minimize memory accesses
  /// performed by CP micro code.

  /// Address of signal used for queue-inactive event notifications
  /// from CP micro code, which is forwarded by KFD to HSA runtime. If
  /// CP detects an error then it sends th error code to the signal
  /// with this address and stops processing further packets. The HSA
  /// Runtime puts the associated HSA queue into the inactive state.
  hsa_signal_t queue_inactive_signal;

  /// Ensures any following data is in a different cache line to the
  /// previous fields that are used by the HSA Component packet
  /// processor. Reserved. Must be 0.
  uint32_t reserved5[14];

} amd_queue_t;

#ifdef __cplusplus
}
#endif

#endif // header guard
