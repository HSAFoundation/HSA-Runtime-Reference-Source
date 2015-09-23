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

#ifndef HSA_RUNTME_CORE_INC_SIGNAL_H_
#define HSA_RUNTME_CORE_INC_SIGNAL_H_

#include "core/inc/runtime.h"
#include "core/inc/checked.h"
#include "core/util/utils.h"

#include "core/inc/thunk.h"

#include "amd_hsa_signal.h"

namespace core {

/// @brief An abstract base class which helps implement the public hsa_signal_t
/// type (an opaque handle) and its associated APIs. At its core, signal uses
/// a 32 or 64 bit value. This value can be waitied on or signaled atomically
/// using specified memory ordering semantics.
class Signal : public Checked<0x71FCCA6A3D5D5276> {
 public:
  /// @brief Constructor initializes the signal with initial value.
  explicit Signal(hsa_signal_value_t initial_value) {
    signal_.kind = AMD_SIGNAL_KIND_INVALID;
    signal_.value = initial_value;
    invalid_ = false;
    waiting_ = 0;
    retained_ = 0;
  }

  virtual ~Signal() { signal_.kind = AMD_SIGNAL_KIND_INVALID; }

  bool IsValid() const {
    if (CheckedType::IsValid() && !invalid_) return true;
    return false;
  }

  /// @brief Converts from this implementation class to the public
  /// hsa_signal_t type - an opaque handle.
  static __forceinline hsa_signal_t Convert(Signal* signal) {
    hsa_signal_t signal_handle = {
        static_cast<uint64_t>(reinterpret_cast<uintptr_t>(&signal->signal_))};
    return signal_handle;
  }

  /// @brief Converts from this implementation class to the public
  /// hsa_signal_t type - an opaque handle.
  static __forceinline const hsa_signal_t Convert(const Signal* signal) {
    hsa_signal_t signal_handle = {
        static_cast<uint64_t>(reinterpret_cast<uintptr_t>(&signal->signal_))};
    return signal_handle;
  }

  /// @brief Converts from public hsa_signal_t type (an opaque handle) to
  /// this implementation class object.
  static __forceinline Signal* Convert(hsa_signal_t signal) {
    return (signal.handle == 0)
               ? NULL
               : reinterpret_cast<Signal*>(
                     static_cast<size_t>(signal.handle) -
                     (reinterpret_cast<size_t>(
                          &(reinterpret_cast<Signal*>(12345)->signal_)) -
                      12345));
  }

  // Below are various methods corresponding to the APIs, which load/store the
  // signal value or modify the existing signal value automically and with
  // specified memory ordering semantics.
  virtual hsa_signal_value_t LoadRelaxed() = 0;
  virtual hsa_signal_value_t LoadAcquire() = 0;

  virtual void StoreRelaxed(hsa_signal_value_t value) = 0;
  virtual void StoreRelease(hsa_signal_value_t value) = 0;

  virtual hsa_signal_value_t WaitRelaxed(hsa_signal_condition_t condition,
                                         hsa_signal_value_t compare_value,
                                         uint64_t timeout,
                                         hsa_wait_state_t wait_hint) = 0;
  virtual hsa_signal_value_t WaitAcquire(hsa_signal_condition_t condition,
                                         hsa_signal_value_t compare_value,
                                         uint64_t timeout,
                                         hsa_wait_state_t wait_hint) = 0;

  virtual void AndRelaxed(hsa_signal_value_t value) = 0;
  virtual void AndAcquire(hsa_signal_value_t value) = 0;
  virtual void AndRelease(hsa_signal_value_t value) = 0;
  virtual void AndAcqRel(hsa_signal_value_t value) = 0;

  virtual void OrRelaxed(hsa_signal_value_t value) = 0;
  virtual void OrAcquire(hsa_signal_value_t value) = 0;
  virtual void OrRelease(hsa_signal_value_t value) = 0;
  virtual void OrAcqRel(hsa_signal_value_t value) = 0;

  virtual void XorRelaxed(hsa_signal_value_t value) = 0;
  virtual void XorAcquire(hsa_signal_value_t value) = 0;
  virtual void XorRelease(hsa_signal_value_t value) = 0;
  virtual void XorAcqRel(hsa_signal_value_t value) = 0;

  virtual void AddRelaxed(hsa_signal_value_t value) = 0;
  virtual void AddAcquire(hsa_signal_value_t value) = 0;
  virtual void AddRelease(hsa_signal_value_t value) = 0;
  virtual void AddAcqRel(hsa_signal_value_t value) = 0;

  virtual void SubRelaxed(hsa_signal_value_t value) = 0;
  virtual void SubAcquire(hsa_signal_value_t value) = 0;
  virtual void SubRelease(hsa_signal_value_t value) = 0;
  virtual void SubAcqRel(hsa_signal_value_t value) = 0;

  virtual hsa_signal_value_t ExchRelaxed(hsa_signal_value_t value) = 0;
  virtual hsa_signal_value_t ExchAcquire(hsa_signal_value_t value) = 0;
  virtual hsa_signal_value_t ExchRelease(hsa_signal_value_t value) = 0;
  virtual hsa_signal_value_t ExchAcqRel(hsa_signal_value_t value) = 0;

  virtual hsa_signal_value_t CasRelaxed(hsa_signal_value_t expected,
                                        hsa_signal_value_t value) = 0;
  virtual hsa_signal_value_t CasAcquire(hsa_signal_value_t expected,
                                        hsa_signal_value_t value) = 0;
  virtual hsa_signal_value_t CasRelease(hsa_signal_value_t expected,
                                        hsa_signal_value_t value) = 0;
  virtual hsa_signal_value_t CasAcqRel(hsa_signal_value_t expected,
                                       hsa_signal_value_t value) = 0;

  //-------------------------
  // implementation specific
  //-------------------------

  /// @brief Returns the address of the value.
  virtual hsa_signal_value_t* ValueLocation() const = 0;

  /// @brief Applies only to InterrupEvent type, returns the event used to.
  /// Returns NULL for DefaultEvent Type.
  virtual HsaEvent* EopEvent() = 0;

  /// @brief Waits until any signal in the list satisfies its condition or
  /// timeout is reached.
  /// Returns the index of a satisfied signal.  Returns -1 on timeout and
  /// errors.
  static uint32_t WaitAny(uint32_t signal_count, hsa_signal_t* hsa_signals,
                          hsa_signal_condition_t* conds,
                          hsa_signal_value_t* values, uint64_t timeout_hint,
                          hsa_wait_state_t wait_hint,
                          hsa_signal_value_t* satisfying_value);

  /// @brief Allows special case interaction with signal destruction cleanup.
  void Retain() { atomic::Increment(&retained_); }
  void Release() { atomic::Decrement(&retained_); }

  /// @brief Checks if signal is currently in use.
  bool InUse() const { return (retained_ != 0) || (waiting_ != 0); }

  /// @brief Structure which defines key signal elements like type and value.
  /// Address of this struct is used as a value for the opaque handle of type
  /// hsa_signal_t provided to the public API.
  amd_signal_t signal_;

 protected:
  /// @variable  Indicates if signal is valid or not.
  volatile bool invalid_;

  /// @variable Indicates number of runtime threads waiting on this signal.
  /// Value of zero means no waits.
  volatile uint32_t waiting_;

  volatile uint32_t retained_;

 private:
  DISALLOW_COPY_AND_ASSIGN(Signal);
};

struct hsa_signal_handle {
  hsa_signal_t signal;

  hsa_signal_handle() {}
  hsa_signal_handle(hsa_signal_t Signal) { signal = Signal; }
  operator hsa_signal_t() { return signal; }
  Signal* operator->() { return core::Signal::Convert(signal); }
};
static_assert(
    sizeof(hsa_signal_handle) == sizeof(hsa_signal_t),
    "hsa_signal_handle and hsa_signal_t must have identical binary layout.");
static_assert(
    sizeof(hsa_signal_handle[2]) == sizeof(hsa_signal_t[2]),
    "hsa_signal_handle and hsa_signal_t must have identical binary layout.");

}  // namespace core
#endif  // header guard
