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

#include "core/inc/default_signal.h"

namespace core {

DefaultSignal::DefaultSignal(hsa_signal_value_t initial_value)
    : Signal(initial_value), invalid_(false), waiting_(0) {
  signal_.type = kHsaSignalAmd;
  signal_.event_mailbox_ptr = NULL;
  hsa_memory_register(this, sizeof(DefaultSignal));
}

DefaultSignal::~DefaultSignal() {
  invalid_ = true;
  while (waiting_ != 0)
    ;
  hsa_memory_deregister(this, sizeof(DefaultSignal));
}

hsa_signal_value_t DefaultSignal::LoadRelaxed() {
  return hsa_signal_value_t(
      atomic::Load(&signal_.value, std::memory_order_relaxed));
}

hsa_signal_value_t DefaultSignal::LoadAcquire() {
  return hsa_signal_value_t(
      atomic::Load(&signal_.value, std::memory_order_acquire));
}

void DefaultSignal::StoreRelaxed(hsa_signal_value_t value) {
  atomic::Store(&signal_.value, int64_t(value), std::memory_order_relaxed);
}

void DefaultSignal::StoreRelease(hsa_signal_value_t value) {
  atomic::Store(&signal_.value, int64_t(value), std::memory_order_release);
}

hsa_signal_value_t DefaultSignal::WaitRelaxed(hsa_signal_condition_t condition,
                                              hsa_signal_value_t compare_value,
                                              uint64_t timeout,
                                              hsa_wait_expectancy_t wait_hint) {
  atomic::Increment(&waiting_);
  MAKE_SCOPE_GUARD([&]() { atomic::Decrement(&waiting_); });
  bool condition_met = false;
  int64_t value;

  uint64_t start_time, sys_time;
  hsa_system_get_info(HSA_SYSTEM_INFO_TIMESTAMP, &start_time);

  while (true) {
    hsa_system_get_info(HSA_SYSTEM_INFO_TIMESTAMP, &sys_time);
    if (sys_time - start_time > timeout) {
      value = atomic::Load(&signal_.value, std::memory_order_relaxed);
      return hsa_signal_value_t(value);
    }

    for (int i = 0; i < 10000; i++) {
      if (invalid_) return 0;

      value = atomic::Load(&signal_.value, std::memory_order_relaxed);

      switch (condition) {
        case HSA_EQ: {
          condition_met = (value == compare_value);
          break;
        }
        case HSA_NE: {
          condition_met = (value != compare_value);
          break;
        }
        case HSA_GTE: {
          condition_met = (value >= compare_value);
          break;
        }
        case HSA_LT: {
          condition_met = (value < compare_value);
          break;
        }
        default:
          return 0;
      }
      if (condition_met) return hsa_signal_value_t(value);
    }
  }
}

hsa_signal_value_t DefaultSignal::WaitAcquire(hsa_signal_condition_t condition,
                                              hsa_signal_value_t compare_value,
                                              uint64_t timeout,
                                              hsa_wait_expectancy_t wait_hint) {
  hsa_signal_value_t ret =
      WaitRelaxed(condition, compare_value, timeout, wait_hint);
  std::atomic_thread_fence(std::memory_order_acquire);
  return ret;
}

void DefaultSignal::AndRelaxed(hsa_signal_value_t value) {
  atomic::And(&signal_.value, int64_t(value), std::memory_order_relaxed);
}

void DefaultSignal::AndAcquire(hsa_signal_value_t value) {
  atomic::And(&signal_.value, int64_t(value), std::memory_order_acquire);
}

void DefaultSignal::AndRelease(hsa_signal_value_t value) {
  atomic::And(&signal_.value, int64_t(value), std::memory_order_release);
}

void DefaultSignal::AndAcqRel(hsa_signal_value_t value) {
  atomic::And(&signal_.value, int64_t(value), std::memory_order_acq_rel);
}

void DefaultSignal::OrRelaxed(hsa_signal_value_t value) {
  atomic::Or(&signal_.value, int64_t(value), std::memory_order_relaxed);
}

void DefaultSignal::OrAcquire(hsa_signal_value_t value) {
  atomic::Or(&signal_.value, int64_t(value), std::memory_order_acquire);
}

void DefaultSignal::OrRelease(hsa_signal_value_t value) {
  atomic::Or(&signal_.value, int64_t(value), std::memory_order_release);
}

void DefaultSignal::OrAcqRel(hsa_signal_value_t value) {
  atomic::Or(&signal_.value, int64_t(value), std::memory_order_acq_rel);
}

void DefaultSignal::XorRelaxed(hsa_signal_value_t value) {
  atomic::Xor(&signal_.value, int64_t(value), std::memory_order_relaxed);
}

void DefaultSignal::XorAcquire(hsa_signal_value_t value) {
  atomic::Xor(&signal_.value, int64_t(value), std::memory_order_acquire);
}

void DefaultSignal::XorRelease(hsa_signal_value_t value) {
  atomic::Xor(&signal_.value, int64_t(value), std::memory_order_release);
}

void DefaultSignal::XorAcqRel(hsa_signal_value_t value) {
  atomic::Xor(&signal_.value, int64_t(value), std::memory_order_acq_rel);
}

void DefaultSignal::AddRelaxed(hsa_signal_value_t value) {
  atomic::Add(&signal_.value, int64_t(value), std::memory_order_relaxed);
}

void DefaultSignal::AddAcquire(hsa_signal_value_t value) {
  atomic::Add(&signal_.value, int64_t(value), std::memory_order_acquire);
}

void DefaultSignal::AddRelease(hsa_signal_value_t value) {
  atomic::Add(&signal_.value, int64_t(value), std::memory_order_release);
}

void DefaultSignal::AddAcqRel(hsa_signal_value_t value) {
  atomic::Add(&signal_.value, int64_t(value), std::memory_order_acq_rel);
}

void DefaultSignal::SubRelaxed(hsa_signal_value_t value) {
  atomic::Sub(&signal_.value, int64_t(value), std::memory_order_relaxed);
}

void DefaultSignal::SubAcquire(hsa_signal_value_t value) {
  atomic::Sub(&signal_.value, int64_t(value), std::memory_order_acquire);
}

void DefaultSignal::SubRelease(hsa_signal_value_t value) {
  atomic::Sub(&signal_.value, int64_t(value), std::memory_order_release);
}

void DefaultSignal::SubAcqRel(hsa_signal_value_t value) {
  atomic::Sub(&signal_.value, int64_t(value), std::memory_order_acq_rel);
}

hsa_signal_value_t DefaultSignal::ExchRelaxed(hsa_signal_value_t value) {
  return hsa_signal_value_t(atomic::Exchange(&signal_.value, int64_t(value),
                                             std::memory_order_relaxed));
}

hsa_signal_value_t DefaultSignal::ExchAcquire(hsa_signal_value_t value) {
  return hsa_signal_value_t(atomic::Exchange(&signal_.value, int64_t(value),
                                             std::memory_order_acquire));
}

hsa_signal_value_t DefaultSignal::ExchRelease(hsa_signal_value_t value) {
  return hsa_signal_value_t(atomic::Exchange(&signal_.value, int64_t(value),
                                             std::memory_order_release));
}

hsa_signal_value_t DefaultSignal::ExchAcqRel(hsa_signal_value_t value) {
  return hsa_signal_value_t(atomic::Exchange(&signal_.value, int64_t(value),
                                             std::memory_order_acq_rel));
}

hsa_signal_value_t DefaultSignal::CasRelaxed(hsa_signal_value_t expected,
                                             hsa_signal_value_t value) {
  return hsa_signal_value_t(atomic::Cas(&signal_.value, int64_t(value),
                                        int64_t(expected),
                                        std::memory_order_relaxed));
}

hsa_signal_value_t DefaultSignal::CasAcquire(hsa_signal_value_t expected,
                                             hsa_signal_value_t value) {
  return hsa_signal_value_t(atomic::Cas(&signal_.value, int64_t(value),
                                        int64_t(expected),
                                        std::memory_order_acquire));
}

hsa_signal_value_t DefaultSignal::CasRelease(hsa_signal_value_t expected,
                                             hsa_signal_value_t value) {
  return hsa_signal_value_t(atomic::Cas(&signal_.value, int64_t(value),
                                        int64_t(expected),
                                        std::memory_order_release));
}

hsa_signal_value_t DefaultSignal::CasAcqRel(hsa_signal_value_t expected,
                                            hsa_signal_value_t value) {
  return hsa_signal_value_t(atomic::Cas(&signal_.value, int64_t(value),
                                        int64_t(expected),
                                        std::memory_order_acq_rel));
}

}  // namespace core