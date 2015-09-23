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
#include "core/util/timer.h"

namespace core {

HsaEvent* InterruptSignal::CreateEvent() {
  HsaEventDescriptor event_descriptor;
#ifdef __linux__
  event_descriptor.EventType = HSA_EVENTTYPE_SIGNAL;
#else
  event_descriptor.EventType = HSA_EVENTTYPE_QUEUE_EVENT;
#endif
  event_descriptor.SyncVar.SyncVar.UserData = NULL;
  event_descriptor.SyncVar.SyncVarSize = sizeof(hsa_signal_value_t);
  event_descriptor.NodeId = 0;
  HsaEvent* ret = NULL;
  hsaKmtCreateEvent(&event_descriptor, false, false, &ret);
  return ret;
}

void InterruptSignal::DestroyEvent(HsaEvent* evt) { hsaKmtDestroyEvent(evt); }

InterruptSignal::InterruptSignal(hsa_signal_value_t initial_value,
                                 HsaEvent* use_event)
    : Signal(initial_value) {
  if (use_event != NULL) {
    event_ = use_event;
    free_event_ = false;
  } else {
    event_ = CreateEvent();
    free_event_ = true;
  }

  if (event_ != NULL) {
    signal_.event_id = event_->EventId;
    signal_.event_mailbox_ptr = event_->EventData.HWData2;
  } else {
    signal_.event_id = 0;
    signal_.event_mailbox_ptr = 0;
  }
  signal_.kind = AMD_SIGNAL_KIND_USER;
  HSA::hsa_memory_register(this, sizeof(InterruptSignal));
}

InterruptSignal::~InterruptSignal() {
  invalid_ = true;
  hsaKmtSetEvent(event_);
  while (InUse())
    ;
  if (free_event_) hsaKmtDestroyEvent(event_);
  HSA::hsa_memory_deregister(this, sizeof(InterruptSignal));
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
    uint64_t timeout, hsa_wait_state_t wait_hint) {
  uint32_t prior = atomic::Increment(&waiting_);

  // assert(prior == 0 && "Multiple waiters on interrupt signal!");
  // Allow only the first waiter to sleep (temporary, known to be bad).
  if (prior != 0) wait_hint = HSA_WAIT_STATE_ACTIVE;

  MAKE_SCOPE_GUARD([&]() { atomic::Decrement(&waiting_); });

  int64_t value;

  timer::fast_clock::time_point start_time = timer::fast_clock::now();

  // Set a polling timeout value
  // Exact time is not hugely important, it should just be a short while which
  // is smaller than the thread scheduling quantum (usually around 16ms)
  const timer::fast_clock::duration kMaxElapsed = std::chrono::milliseconds(5);

  uint64_t hsa_freq;
  HSA::hsa_system_get_info(HSA_SYSTEM_INFO_TIMESTAMP_FREQUENCY, &hsa_freq);
  const timer::fast_clock::duration fast_timeout =
      timer::duration_from_seconds<timer::fast_clock::duration>(
          double(timeout) / double(hsa_freq));

  bool condition_met = false;
  while (true) {
    if (invalid_) return 0;

    value = atomic::Load(&signal_.value, std::memory_order_relaxed);

    switch (condition) {
      case HSA_SIGNAL_CONDITION_EQ: {
        condition_met = (value == compare_value);
        break;
      }
      case HSA_SIGNAL_CONDITION_NE: {
        condition_met = (value != compare_value);
        break;
      }
      case HSA_SIGNAL_CONDITION_GTE: {
        condition_met = (value >= compare_value);
        break;
      }
      case HSA_SIGNAL_CONDITION_LT: {
        condition_met = (value < compare_value);
        break;
      }
      default:
        return 0;
    }
    if (condition_met) return hsa_signal_value_t(value);

    timer::fast_clock::time_point time = timer::fast_clock::now();
    if (time - start_time > kMaxElapsed) {
      if (time - start_time > fast_timeout) {
        value = atomic::Load(&signal_.value, std::memory_order_relaxed);
        return hsa_signal_value_t(value);
      }
      if (wait_hint != HSA_WAIT_STATE_ACTIVE) {
        uint32_t wait_ms;
        auto time_remaining = fast_timeout - (time - start_time);
        if ((timeout == -1) ||
            (time_remaining > std::chrono::milliseconds(uint32_t(-1))))
          wait_ms = uint32_t(-1);
        else
          wait_ms = timer::duration_cast<std::chrono::milliseconds>(
                        time_remaining).count();
        hsaKmtWaitOnEvent(event_, wait_ms);
      }
    }
  }
}

hsa_signal_value_t InterruptSignal::WaitAcquire(
    hsa_signal_condition_t condition, hsa_signal_value_t compare_value,
    uint64_t timeout, hsa_wait_state_t wait_hint) {
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
  hsa_signal_value_t ret = hsa_signal_value_t(atomic::Exchange(
      &signal_.value, int64_t(value), std::memory_order_relaxed));
  hsaKmtSetEvent(event_);
  return ret;
}

hsa_signal_value_t InterruptSignal::ExchAcquire(hsa_signal_value_t value) {
  hsa_signal_value_t ret = hsa_signal_value_t(atomic::Exchange(
      &signal_.value, int64_t(value), std::memory_order_acquire));
  hsaKmtSetEvent(event_);
  return ret;
}

hsa_signal_value_t InterruptSignal::ExchRelease(hsa_signal_value_t value) {
  hsa_signal_value_t ret = hsa_signal_value_t(atomic::Exchange(
      &signal_.value, int64_t(value), std::memory_order_release));
  hsaKmtSetEvent(event_);
  return ret;
}

hsa_signal_value_t InterruptSignal::ExchAcqRel(hsa_signal_value_t value) {
  hsa_signal_value_t ret = hsa_signal_value_t(atomic::Exchange(
      &signal_.value, int64_t(value), std::memory_order_acq_rel));
  hsaKmtSetEvent(event_);
  return ret;
}

hsa_signal_value_t InterruptSignal::CasRelaxed(hsa_signal_value_t expected,
                                               hsa_signal_value_t value) {
  hsa_signal_value_t ret = hsa_signal_value_t(
      atomic::Cas(&signal_.value, int64_t(value), int64_t(expected),
                  std::memory_order_relaxed));
  hsaKmtSetEvent(event_);
  return ret;
}

hsa_signal_value_t InterruptSignal::CasAcquire(hsa_signal_value_t expected,
                                               hsa_signal_value_t value) {
  hsa_signal_value_t ret = hsa_signal_value_t(
      atomic::Cas(&signal_.value, int64_t(value), int64_t(expected),
                  std::memory_order_acquire));
  hsaKmtSetEvent(event_);
  return ret;
}

hsa_signal_value_t InterruptSignal::CasRelease(hsa_signal_value_t expected,
                                               hsa_signal_value_t value) {
  hsa_signal_value_t ret = hsa_signal_value_t(
      atomic::Cas(&signal_.value, int64_t(value), int64_t(expected),
                  std::memory_order_release));
  hsaKmtSetEvent(event_);
  return ret;
}

hsa_signal_value_t InterruptSignal::CasAcqRel(hsa_signal_value_t expected,
                                              hsa_signal_value_t value) {
  hsa_signal_value_t ret = hsa_signal_value_t(
      atomic::Cas(&signal_.value, int64_t(value), int64_t(expected),
                  std::memory_order_acq_rel));
  hsaKmtSetEvent(event_);
  return ret;
}

}  // namespace core
