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

#include "core/util/timer.h"

namespace timer {

accurate_clock::init::init() {
  freq = os::AccurateClockFrequency();
  accurate_clock::period_ns = 1e9 / double(freq);
}

// Calibrates the fast clock using the accurate clock.
fast_clock::init::init() {
  typedef accurate_clock clock;
  clock::duration delay(std::chrono::milliseconds(1));

  // calibrate clock
  fast_clock::raw_rep min = 0;
  clock::duration elapsed = clock::duration::max();

  do {
    for (int t = 0; t < 10; t++) {
      fast_clock::raw_rep r1, r2;
      clock::time_point t0, t1, t2, t3;

      t0 = clock::now();
      std::atomic_signal_fence(std::memory_order_acq_rel);
      r1 = fast_clock::raw_now();
      std::atomic_signal_fence(std::memory_order_acq_rel);
      t1 = clock::now();
      std::atomic_signal_fence(std::memory_order_acq_rel);

      do {
        t2 = clock::now();
      } while (t2 - t1 < delay);

      std::atomic_signal_fence(std::memory_order_acq_rel);
      r2 = fast_clock::raw_now();
      std::atomic_signal_fence(std::memory_order_acq_rel);
      t3 = clock::now();

      // If elapsed time is shorter than last recorded time and both the start
      // and end times are confirmed correlated then record the clock readings.
      // This protects against inaccuracy due to thread switching
      if ((t3 - t1 < elapsed) && ((t1 - t0) * 10 < (t2 - t1)) &&
          ((t3 - t2) * 10 < (t2 - t1))) {
        elapsed = t3 - t1;
        min = r2 - r1;
      }
    }
    delay += delay;
  } while (min < 1000);

  fast_clock::freq = double(min) / duration_in_seconds(elapsed);
  fast_clock::period_ps = 1e12 / fast_clock::freq;
}

double accurate_clock::period_ns;
accurate_clock::raw_frequency accurate_clock::freq;
accurate_clock::init accurate_clock::accurate_clock_init;

double fast_clock::period_ps;
fast_clock::raw_frequency fast_clock::freq;
fast_clock::init fast_clock::fast_clock_init;
}
