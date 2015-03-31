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

#ifndef HSA_RUNTME_CORE_INC_INTERRUPT_SIGNAL_H_
#define HSA_RUNTME_CORE_INC_INTERRUPT_SIGNAL_H_

#include "core/inc/runtime.h"
#include "core/inc/signal.h"
#include "core/util/utils.h"

#include "core/inc/thunk.h"

namespace core {

/// @brief A Signal implementation using interrupts versus plain memory based.
/// Also see base class Signal.
///
/// Breaks common/vendor separation - signals in general needs to be re-worked
/// at the foundation level to make sense in a multi-device system.
/// Supports only one waiter for now.
/// KFD changes are needed to support multiple waiters and have device
/// signaling.
class InterruptSignal : public Signal {
 public:
  explicit InterruptSignal(hsa_signal_value_t initial_value);

  ~InterruptSignal();

  /// @brief Factory method to create an instance of this class.
  /// @param initial_value(input), Initial value of the signal.
  /// @param num_consumers(input), Number of agents that can wait on
  /// this signal.
  /// @param consumers(input), List of agents that might consume (wait on)
  /// the signal. Must match the num_consumers param.
  static InterruptSignal* Create(hsa_signal_value_t initial_value,
                                 uint32_t num_consumers,
                                 const hsa_agent_t* consumers);

  // Below are various methods corresponding to the APIs, which load/store the
  // signal value or modify the existing signal value automically and with
  // specified memory ordering semantics.

  hsa_signal_value_t LoadRelaxed();

  hsa_signal_value_t LoadAcquire();

  void StoreRelaxed(hsa_signal_value_t value);

  void StoreRelease(hsa_signal_value_t value);

  hsa_signal_value_t WaitRelaxed(hsa_signal_condition_t condition,
                                 hsa_signal_value_t compare_value,
                                 uint64_t timeout,
                                 hsa_wait_expectancy_t wait_hint);

  hsa_signal_value_t WaitAcquire(hsa_signal_condition_t condition,
                                 hsa_signal_value_t compare_value,
                                 uint64_t timeout,
                                 hsa_wait_expectancy_t wait_hint);

  void AndRelaxed(hsa_signal_value_t value);

  void AndAcquire(hsa_signal_value_t value);

  void AndRelease(hsa_signal_value_t value);

  void AndAcqRel(hsa_signal_value_t value);

  void OrRelaxed(hsa_signal_value_t value);

  void OrAcquire(hsa_signal_value_t value);

  void OrRelease(hsa_signal_value_t value);

  void OrAcqRel(hsa_signal_value_t value);

  void XorRelaxed(hsa_signal_value_t value);

  void XorAcquire(hsa_signal_value_t value);

  void XorRelease(hsa_signal_value_t value);

  void XorAcqRel(hsa_signal_value_t value);

  void AddRelaxed(hsa_signal_value_t value);

  void AddAcquire(hsa_signal_value_t value);

  void AddRelease(hsa_signal_value_t value);

  void AddAcqRel(hsa_signal_value_t value);

  void SubRelaxed(hsa_signal_value_t value);

  void SubAcquire(hsa_signal_value_t value);

  void SubRelease(hsa_signal_value_t value);

  void SubAcqRel(hsa_signal_value_t value);

  hsa_signal_value_t ExchRelaxed(hsa_signal_value_t value);

  hsa_signal_value_t ExchAcquire(hsa_signal_value_t value);

  hsa_signal_value_t ExchRelease(hsa_signal_value_t value);

  hsa_signal_value_t ExchAcqRel(hsa_signal_value_t value);

  hsa_signal_value_t CasRelaxed(hsa_signal_value_t expected,
                                hsa_signal_value_t value);

  hsa_signal_value_t CasAcquire(hsa_signal_value_t expected,
                                hsa_signal_value_t value);

  hsa_signal_value_t CasRelease(hsa_signal_value_t expected,
                                hsa_signal_value_t value);

  hsa_signal_value_t CasAcqRel(hsa_signal_value_t expected,
                               hsa_signal_value_t value);

  /// @brief See base class Signal.
  __forceinline hsa_signal_value_t* ValueLocation() const {
    return (hsa_signal_value_t*)&signal_.value;
  }

  /// @brief See base class Signal.
  __forceinline HsaEvent* EopEvent() { return event_; }

  /// @brief prevent throwing exceptions
  void* operator new(size_t size) { return malloc(size); }

  /// @brief prevent throwing exceptions
  void operator delete(void* ptr) { free(ptr); }

 private:
  /// @variable KFD event on which the interrupt signal is based on.
  HsaEvent* event_;

  /// @variable Indicates that signal is destroyed.
  volatile bool invalid_;

  /// @variable Number of threads currently waiting on this signal.
  volatile uint32_t waiting_;

  DISALLOW_COPY_AND_ASSIGN(InterruptSignal);
};

}  // namespace core
#endif  // header guard
