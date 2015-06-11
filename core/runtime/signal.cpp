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

#ifndef HSA_RUNTME_CORE_SIGNAL_CPP_
#define HSA_RUNTME_CORE_SIGNAL_CPP_

#include "core/inc/signal.h"
#include "core/util/timer.h"
#include <algorithm>

namespace core {

uint32_t Signal::WaitAny(uint32_t signal_count, hsa_signal_t* hsa_signals,
                         hsa_signal_condition_t* conds,
                         hsa_signal_value_t* values, uint64_t timeout,
                         hsa_wait_state_t wait_hint,
                         hsa_signal_value_t* satisfying_value) {
  hsa_signal_handle* signals =
      reinterpret_cast<hsa_signal_handle*>(hsa_signals);
  uint32_t prior = 0;
  for (uint32_t i = 0; i < signal_count; i++)
    prior = Max(prior, atomic::Increment(&signals[i]->waiting_));

  MAKE_SCOPE_GUARD([&]() {
    for (uint32_t i = 0; i < signal_count; i++)
      atomic::Decrement(&signals[i]->waiting_);
  });

  // Allow only the first waiter to sleep (temporary, known to be bad).
  if (prior != 0) wait_hint = HSA_WAIT_STATE_ACTIVE;

  // Ensure that all signals in the list can be slept on.
  if (wait_hint != HSA_WAIT_STATE_ACTIVE) {
    for (uint32_t i = 0; i < signal_count; i++) {
      if (signals[i]->EopEvent() == NULL) {
        wait_hint = HSA_WAIT_STATE_ACTIVE;
        break;
      }
    }
  }

  const uint32_t small_size = 10;
  HsaEvent* short_evts[small_size];
  HsaEvent** evts = NULL;
  uint32_t unique_evts = 0;
  if (wait_hint != HSA_WAIT_STATE_ACTIVE) {
    if (signal_count > small_size)
      evts = new HsaEvent* [signal_count];
    else
      evts = short_evts;
    for (uint32_t i = 0; i < signal_count; i++)
      evts[i] = signals[i]->EopEvent();
    std::sort(evts, evts + signal_count);
    HsaEvent** end = std::unique(evts, evts + signal_count);
    unique_evts = uint32_t(end - evts);
  }
  MAKE_SCOPE_GUARD([&]() {
    if (signal_count > small_size) delete[] evts;
  });

  int64_t value;

  timer::fast_clock::time_point start_time = timer::fast_clock::now();

  // Set a polling timeout value
  // Exact time is not hugely important, it should just be a short while which
  // is smaller than the thread scheduling quantum (usually around 16ms)
  const timer::fast_clock::duration kMaxElapsed = std::chrono::milliseconds(5);

  // Convert timeout value into the fast_clock domain
  uint64_t hsa_freq;
  HSA::hsa_system_get_info(HSA_SYSTEM_INFO_TIMESTAMP_FREQUENCY, &hsa_freq);
  const timer::fast_clock::duration fast_timeout =
      timer::duration_from_seconds<timer::fast_clock::duration>(
          double(timeout) / double(hsa_freq));

  bool condition_met = false;
  while (true) {
    for (uint32_t i = 0; i < signal_count; i++) {
      if (signals[i]->invalid_) return uint32_t(-1);

      value =
          atomic::Load(&signals[i]->signal_.value, std::memory_order_relaxed);

      switch (conds[i]) {
        case HSA_SIGNAL_CONDITION_EQ: {
          condition_met = (value == values[i]);
          break;
        }
        case HSA_SIGNAL_CONDITION_NE: {
          condition_met = (value != values[i]);
          break;
        }
        case HSA_SIGNAL_CONDITION_GTE: {
          condition_met = (value >= values[i]);
          break;
        }
        case HSA_SIGNAL_CONDITION_LT: {
          condition_met = (value < values[i]);
          break;
        }
        default:
          return uint32_t(-1);
      }
      if (condition_met) {
        if (satisfying_value != NULL) *satisfying_value = value;
        return i;
      }
    }

    timer::fast_clock::time_point time = timer::fast_clock::now();
    if (time - start_time > kMaxElapsed) {
      if (time - start_time > fast_timeout) {
        return uint32_t(-1);
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
        hsaKmtWaitOnMultipleEvents(evts, unique_evts, false, wait_ms);
      }
    }
  }
}

}  // namespace core

#endif  // header guard
