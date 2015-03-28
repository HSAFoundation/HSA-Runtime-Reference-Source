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

#ifndef HSA_RUNTME_CORE_INC_THUNK_H_
#define HSA_RUNTME_CORE_INC_THUNK_H_

// Add path to hsakmt.h in include search path in CMakeLists.txt or 
// any in other build framework being used.
#include "hsakmt.h"
#include "core/util/utils.h"

typedef struct HsaQueueResourceFixed {
  HSA_QUEUEID queue_id;  // queue ID
  // Doorbell address to notify HW of a new dispatch
  union {
    volatile HSAuint32* queue_doorbell;
    HSAuint64 queue_door_bell;
  };

  // virtual address to notify HW of queue write ptr value
  union {
    volatile HSAuint32* queue_writeptr;
    HSAuint64 queue_writeptr_value;
  };

  // virtual address updated by HW to indicate current read location
  union {
    volatile HSAuint32* queue_readptr;
    HSAuint64 queue_readptr_value;
  };
} HsaQueueResourceFixed;

static __forceinline HSAKMT_STATUS
    hsaKmtCreateQueue(HSAuint32 node_id, HSA_QUEUE_TYPE type,
                      HSAuint32 queue_percentage, HSA_QUEUE_PRIORITY priority,
                      void* queue_address, HSAuint64 queue_size,
                      HsaEvent* event, HsaQueueResourceFixed* queue_resource) {
  return hsaKmtCreateQueue(node_id, type, queue_percentage, priority,
                           queue_address, queue_size, event,
                           (HsaQueueResource*)queue_resource);
}
#endif  // header guard