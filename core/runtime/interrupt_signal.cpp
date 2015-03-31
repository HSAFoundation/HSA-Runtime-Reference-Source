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

#include "core/inc/interrupt_signal.h"

namespace core {
InterruptSignal::InterruptSignal(hsa_signal_value_t initial_value)
    : Signal(initial_value), invalid_(false), waiting_(0) {
  HsaEventDescriptor event_descriptor;
#ifdef __linux__
  event_descriptor.EventType = HSA_EVENTTYPE_SIGNAL;
#else
  event_descriptor.EventType = HSA_EVENTTYPE_QUEUE_EVENT;
#endif
  event_descriptor.SyncVar.SyncVar.UserData = (void*)&signal_.value;
  event_descriptor.SyncVar.SyncVarSize = sizeof(hsa_signal_value_t);
  event_descriptor.NodeId = 0;
  if (hsaKmtCreateEvent(&event_descriptor, false, false, &event_) !=
      HSAKMT_STATUS_SUCCESS) {
    event_ = NULL;
    return;
  }
  signal_.type = kHsaSignalAmd;
  signal_.event_id = event_->EventId;
  signal_.event_mailbox_ptr = event_->EventData.HWData2;

  hsa_memory_register(this, sizeof(InterruptSignal));
}

InterruptSignal::~InterruptSignal() {
  invalid_ = true;
  hsaKmtSetEvent(event_);
  while (waiting_ != 0)
    ;
  hsaKmtDestroyEvent(event_);
  hsa_memory_deregister(this, sizeof(InterruptSignal));
}

InterruptSignal* InterruptSignal::Create(hsa_signal_value_t initial_value,
                                         uint32_t num_consumers,
                                         const hsa_agent_t* consumers) {
  InterruptSignal* signal = new InterruptSignal(initial_value);
  if (signal != NULL) {
    if (signal->EopEvent() != NULL) return signal;
  }
  delete signal;
  return NULL;
}

hsa_signal_value_t InterruptSignal::LoadRelaxed() {
  return hsa_signal_value_t(
      atomic::Load(&signal_.value, std::memory_order_relaxed));
}

hsa_signal_value_t InterruptSignal::LoadAcquire() {
  return hsa_signal_value_t(
      atomic::Load(&signal_.value, std::memory_order_acquire));
}

void InterruptSignal::StoreRelaxed(hsa_signal_value_t value) {
  atomic::Store(&signal_.value, int64_t(value), std::memory_order_relaxed);
  hsaKmtSetEvent(event_);
}

void InterruptSignal::StoreRelease(hsa_signal_value_t value) {
  atomic::Store(&signal_.value, int64_t(value), std::memory_order_release);
  hsaKmtSetEvent(event_);
}

hsa_signal_value_t InterruptSignal::WaitRelaxed(
    hsa_signal_condition_t condition, hsa_signal_value_t compare_value,
    uint64_t timeout, hsa_wait_expectancy_t wait_hint) {
  uint32_t prior = atomic::Increment(&waiting_);
  assert(prior == 0 && "Multiple waiters on interrupt signal!");
  MAKE_SCOPE_GUARD([&]() { atomic::Decrement(&waiting_); });

  int64_t value;

  uint64_t fast_start_time = __rdtsc();
  //~200us at 4GHz - does not need to be an exact time, just a short while
  const uint64_t kMaxElapsed = 800000;

  uint64_t hsa_freq;
  hsa_system_get_info(HSA_SYSTEM_INFO_TIMESTAMP_FREQUENCY, &hsa_freq);
  const float invFreq = 1000.0f / hsa_freq;  // clock period in ms

  uint64_t start_time, sys_time;
  hsa_system_get_info(HSA_SYSTEM_INFO_TIMESTAMP, &start_time);

  bool condition_met = false;
  while (true) {
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

    uint64_t time = __rdtsc();
    if (time - fast_start_time > kMaxElapsed) {
      hsa_system_get_info(HSA_SYSTEM_INFO_TIMESTAMP, &sys_time);
      if (sys_time - start_time > timeout) {
        value = atomic::Load(&signal_.value, std::memory_order_relaxed);
        return hsa_signal_value_t(value);
      }
      if (wait_hint != HSA_WAIT_EXPECTANCY_SHORT) {
        uint32_t wait_ms;
        if (timeout == -1)
          wait_ms = uint32_t(-1);
        else
          wait_ms = uint32_t((timeout - (sys_time - start_time)) * invFreq);
        hsaKmtWaitOnEvent(event_, wait_ms);
      }
    }
  }
}

hsa_signal_value_t InterruptSignal::WaitAcquire(
    hsa_signal_condition_t condition, hsa_signal_value_t compare_value,
    uint64_t timeout, hsa_wait_expectancy_t wait_hint) {
  hsa_signal_value_t ret =
      WaitRelaxed(condition, compare_value, timeout, wait_hint);
  std::atomic_thread_fence(std::memory_order_acquire);
  return ret;
}

void InterruptSignal::AndRelaxed(hsa_signal_value_t value) {
  atomic::And(&signal_.value, int64_t(value), std::memory_order_relaxed);
  hsaKmtSetEvent(event_);
}

void InterruptSignal::AndAcquire(hsa_signal_value_t value) {
  atomic::And(&signal_.value, int64_t(value), std::memory_order_acquire);
  hsaKmtSetEvent(event_);
}

void InterruptSignal::AndRelease(hsa_signal_value_t value) {
  atomic::And(&signal_.value, int64_t(value), std::memory_order_release);
  hsaKmtSetEvent(event_);
}

void InterruptSignal::AndAcqRel(hsa_signal_value_t value) {
  atomic::And(&signal_.value, int64_t(value), std::memory_order_acq_rel);
  hsaKmtSetEvent(event_);
}

void InterruptSignal::OrRelaxed(hsa_signal_value_t value) {
  atomic::Or(&signal_.value, int64_t(value), std::memory_order_relaxed);
  hsaKmtSetEvent(event_);
}

void InterruptSignal::OrAcquire(hsa_signal_value_t value) {
  atomic::Or(&signal_.value, int64_t(value), std::memory_order_acquire);
  hsaKmtSetEvent(event_);
}

void InterruptSignal::OrRelease(hsa_signal_value_t value) {
  atomic::Or(&signal_.value, int64_t(value), std::memory_order_release);
  hsaKmtSetEvent(event_);
}

void InterruptSignal::OrAcqRel(hsa_signal_value_t value) {
  atomic::Or(&signal_.value, int64_t(value), std::memory_order_acq_rel);
  hsaKmtSetEvent(event_);
}

void InterruptSignal::XorRelaxed(hsa_signal_value_t value) {
  atomic::Xor(&signal_.value, int64_t(value), std::memory_order_relaxed);
  hsaKmtSetEvent(event_);
}

void InterruptSignal::XorAcquire(hsa_signal_value_t value) {
  atomic::Xor(&signal_.value, int64_t(value), std::memory_order_acquire);
  hsaKmtSetEvent(event_);
}

void InterruptSignal::XorRelease(hsa_signal_value_t value) {
  atomic::Xor(&signal_.value, int64_t(value), std::memory_order_release);
  hsaKmtSetEvent(event_);
}

void InterruptSignal::XorAcqRel(hsa_signal_value_t value) {
  atomic::Xor(&signal_.value, int64_t(value), std::memory_order_acq_rel);
  hsaKmtSetEvent(event_);
}

void InterruptSignal::AddRelaxed(hsa_signal_value_t value) {
  atomic::Add(&signal_.value, int64_t(value), std::memory_order_relaxed);
  hsaKmtSetEvent(event_);
}

void InterruptSignal::AddAcquire(hsa_signal_value_t value) {
  atomic::Add(&signal_.value, int64_t(value), std::memory_order_acquire);
  hsaKmtSetEvent(event_);
}

void InterruptSignal::AddRelease(hsa_signal_value_t value) {
  atomic::Add(&signal_.value, int64_t(value), std::memory_order_release);
  hsaKmtSetEvent(event_);
}

void InterruptSignal::AddAcqRel(hsa_signal_value_t value) {
  atomic::Add(&signal_.value, int64_t(value), std::memory_order_acq_rel);
  hsaKmtSetEvent(event_);
}

void InterruptSignal::SubRelaxed(hsa_signal_value_t value) {
  atomic::Sub(&signal_.value, int64_t(value), std::memory_order_relaxed);
  hsaKmtSetEvent(event_);
}

void InterruptSignal::SubAcquire(hsa_signal_value_t value) {
  atomic::Sub(&signal_.value, int64_t(value), std::memory_order_acquire);
  hsaKmtSetEvent(event_);
}

void InterruptSignal::SubRelease(hsa_signal_value_t value) {
  atomic::Sub(&signal_.value, int64_t(value), std::memory_order_release);
  hsaKmtSetEvent(event_);
}

void InterruptSignal::SubAcqRel(hsa_signal_value_t value) {
  atomic::Sub(&signal_.value, int64_t(value), std::memory_order_acq_rel);
  hsaKmtSetEvent(event_);
}

hsa_signal_value_t InterruptSignal::ExchRelaxed(hsa_signal_value_t value) {
  return hsa_signal_value_t(atomic::Exchange(&signal_.value, int64_t(value),
                                             std::memory_order_relaxed));
  hsaKmtSetEvent(event_);
}

hsa_signal_value_t InterruptSignal::ExchAcquire(hsa_signal_value_t value) {
  return hsa_signal_value_t(atomic::Exchange(&signal_.value, int64_t(value),
                                             std::memory_order_acquire));
  hsaKmtSetEvent(event_);
}

hsa_signal_value_t InterruptSignal::ExchRelease(hsa_signal_value_t value) {
  return hsa_signal_value_t(atomic::Exchange(&signal_.value, int64_t(value),
                                             std::memory_order_release));
  hsaKmtSetEvent(event_);
}

hsa_signal_value_t InterruptSignal::ExchAcqRel(hsa_signal_value_t value) {
  return hsa_signal_value_t(atomic::Exchange(&signal_.value, int64_t(value),
                                             std::memory_order_acq_rel));
  hsaKmtSetEvent(event_);
}

hsa_signal_value_t InterruptSignal::CasRelaxed(hsa_signal_value_t expected,
                                               hsa_signal_value_t value) {
  return hsa_signal_value_t(atomic::Cas(&signal_.value, int64_t(value),
                                        int64_t(expected),
                                        std::memory_order_relaxed));
  hsaKmtSetEvent(event_);
}

hsa_signal_value_t InterruptSignal::CasAcquire(hsa_signal_value_t expected,
                                               hsa_signal_value_t value) {
  return hsa_signal_value_t(atomic::Cas(&signal_.value, int64_t(value),
                                        int64_t(expected),
                                        std::memory_order_acquire));
  hsaKmtSetEvent(event_);
}

hsa_signal_value_t InterruptSignal::CasRelease(hsa_signal_value_t expected,
                                               hsa_signal_value_t value) {
  return hsa_signal_value_t(atomic::Cas(&signal_.value, int64_t(value),
                                        int64_t(expected),
                                        std::memory_order_release));
  hsaKmtSetEvent(event_);
}

hsa_signal_value_t InterruptSignal::CasAcqRel(hsa_signal_value_t expected,
                                              hsa_signal_value_t value) {
  return hsa_signal_value_t(atomic::Cas(&signal_.value, int64_t(value),
                                        int64_t(expected),
                                        std::memory_order_acq_rel));
  hsaKmtSetEvent(event_);
}

}  // namespace core
