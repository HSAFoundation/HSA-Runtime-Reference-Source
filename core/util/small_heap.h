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

// A simple first fit memory allocator with eager compaction.  For use with few
// items (where list iteration is faster than trees).
// Not thread safe!

#ifndef HSA_RUNTME_CORE_UTIL_SMALL_HEAP_H_
#define HSA_RUNTME_CORE_UTIL_SMALL_HEAP_H_

#include "utils.h"

#include <map>

class SmallHeap {
 public:
  class Node {
   public:
    size_t len;
    void* next_free;
    void* prior_free;
    static const intptr_t END = -1;

    __forceinline bool isfree() const { return next_free != NULL; }
    __forceinline bool islastfree() const { return intptr_t(next_free) == END; }
    __forceinline bool isfirstfree() const {
      return intptr_t(prior_free) == END;
    }
    __forceinline void setlastfree() {
      *reinterpret_cast<intptr_t*>(&next_free) = END;
    }
    __forceinline void setfirstfree() {
      *reinterpret_cast<intptr_t*>(&prior_free) = END;
    }
  };

 private:
  SmallHeap(const SmallHeap& rhs);
  SmallHeap& operator=(const SmallHeap& rhs);

  void* const pool;
  const size_t length;

  size_t total_free;
  void* first_free;
  std::map<void*, Node> memory;

  typedef decltype(memory) memory_t;
  memory_t::iterator merge(memory_t::iterator& keep,
                           memory_t::iterator& destroy);

 public:
  SmallHeap() : pool(NULL), length(0), total_free(0) {}
  SmallHeap(void* base, size_t length)
      : pool(base), length(length), total_free(length) {
    first_free = pool;

    Node& node = memory[first_free];
    node.len = length;
    node.setlastfree();
    node.setfirstfree();

    memory[0].len = 0;
    memory[(void*)0xFFFFFFFFFFFFFFFFull].len = 0;
  }

  void* alloc(size_t bytes);
  void free(void* ptr);

  void* base() const { return pool; }
  size_t size() const { return length; }
  size_t remaining() const { return total_free; }
};

#endif