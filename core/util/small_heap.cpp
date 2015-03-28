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

#include "small_heap.h"

SmallHeap::memory_t::iterator SmallHeap::merge(
    SmallHeap::memory_t::iterator& keep,
    SmallHeap::memory_t::iterator& destroy) {
  assert((char*)keep->first + keep->second.len == (char*)destroy->first &&
         "Invalid merge");
  assert(keep->second.isfree() && "Merge with allocated block");
  assert(destroy->second.isfree() && "Merge with allocated block");

  keep->second.len += destroy->second.len;
  keep->second.next_free = destroy->second.next_free;
  if (!destroy->second.islastfree())
    memory[destroy->second.next_free].prior_free = keep->first;

  memory.erase(destroy);
  return keep;
}

void SmallHeap::free(void* ptr) {
  if (ptr == NULL) return;

  auto iterator = memory.find(ptr);

  // Check for illegal free
  if (iterator == memory.end()) {
    assert(false && "Illegal free.");
    return;
  }

  const auto start_guard = memory.find(0);
  const auto end_guard = memory.find((void*)0xFFFFFFFFFFFFFFFFull);

  // Return memory to total and link node into free list
  total_free += iterator->second.len;
  if (first_free < iterator->first) {
    auto before = iterator;
    before--;
    while (before != start_guard && !before->second.isfree()) before--;
    assert(before->second.next_free > iterator->first &&
           "Inconsistency in small heap.");
    iterator->second.prior_free = before->first;
    iterator->second.next_free = before->second.next_free;
    before->second.next_free = iterator->first;
    if (!iterator->second.islastfree())
      memory[iterator->second.next_free].prior_free = iterator->first;
  } else {
    iterator->second.setfirstfree();
    iterator->second.next_free = first_free;
    first_free = iterator->first;
    if (!iterator->second.islastfree())
      memory[iterator->second.next_free].prior_free = iterator->first;
  }

  // Attempt compaction
  auto before = iterator;
  before--;
  if (before != start_guard) {
    if (before->second.isfree()) {
      iterator = merge(before, iterator);
    }
  }

  auto after = iterator;
  after++;
  if (after != end_guard) {
    if (after->second.isfree()) {
      iterator = merge(iterator, after);
    }
  }
}

void* SmallHeap::alloc(size_t bytes) {
  // Is enough memory available?
  if ((bytes > total_free) || (bytes == 0)) return NULL;

  memory_t::iterator current;
  memory_t::iterator prior;

  // Walk the free list and allocate at first fitting location
  prior = current = memory.find(first_free);
  while (true) {
    if (bytes <= current->second.len) {
      // Decrement from total
      total_free -= bytes;

      // Is allocation an exact fit?
      if (bytes == current->second.len) {
        if (prior == current) {
          first_free = current->second.next_free;
          if (!current->second.islastfree())
            memory[current->second.next_free].setfirstfree();
        } else {
          prior->second.next_free = current->second.next_free;
          if (!current->second.islastfree())
            memory[current->second.next_free].prior_free = prior->first;
        }
        current->second.next_free = NULL;
        return current->first;
      } else {
        // Split current node
        void* remaining = (char*)current->first + bytes;
        Node& node = memory[remaining];
        node.next_free = current->second.next_free;
        node.prior_free = current->second.prior_free;
        node.len = current->second.len - bytes;
        current->second.len = bytes;

        if (prior == current) {
          first_free = remaining;
          node.setfirstfree();
        } else {
          prior->second.next_free = remaining;
          node.prior_free = prior->first;
        }
        if (!node.islastfree()) memory[node.next_free].prior_free = remaining;

        current->second.next_free = NULL;
        return current->first;
      }
    }

    // End of free list?
    if (current->second.islastfree()) break;

    prior = current;
    current = memory.find(current->second.next_free);
  }

  // Can't service the request due to fragmentation
  return NULL;
}
