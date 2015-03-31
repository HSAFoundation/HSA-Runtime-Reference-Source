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

// Memory_Datebase.cpp
// Implementation of class Memory_Database.

#include "core/inc/memory_database.h"

#include "core/inc/runtime.h"

namespace core {

// Check if the given address is in the page range or registered. If it is,
// return ture.
bool MemoryDatabase::FindContainingBlock(
    uintptr_t address, std::map<uintptr_t, PageRange>::iterator& near_hint) {
  // Block is prior to near_hint
  if (address < near_hint->first) {
    while (address < near_hint->first) near_hint--;
    if (address < near_hint->first + near_hint->second.size_) return true;
    return false;
  }
  // Block is at or after near_hint
  while (near_hint->first + near_hint->second.size_ <= address) near_hint++;
  if (near_hint->first <= address) return true;
  return false;
}

bool MemoryDatabase::RegisterImpl(void* ptr, size_t size) {
  // Check for zero length, NULL pointer, and pointer overflow.
  if ((size == 0) || (ptr == NULL) || ((uintptr_t)ptr + size < (uintptr_t)ptr))
    return false;

  const uintptr_t base = (uintptr_t)ptr;

  // variable: start_page is the address of the page which base belongs to
  const uintptr_t start_page = GetPage(base);
  // variable: end_page is the address of the page which is imediately after the
  // requested block.
  const uintptr_t end_page = GetNextPage(base, size);

  // Provisionally insert the range - must dismiss Guard in order to keep the
  // range.
  // std::map::insert will return pair<iterator, bool> data type, if the key is
  // already existing, bool is false.
  // variable: provisional_iterator is an iterator to the element whose "key" is
  // equal to the value of base.
  auto provisional_iterator =
      requested_ranges_.insert(std::pair<uintptr_t, Range>(base, Range()))
          .first;
  // variable: temp_iterator is used for checking requested region overlap.
  auto temp_iterator = provisional_iterator;

  // variable: range is a refernce to the value of map
  Range& range = temp_iterator->second;

  // Checks for basic validity of the requested range at all exit points
  // Will assert on new range insertion failure (even though spec defines this
  // as a recoverable error)
  // since this is always a program error.
  MAKE_SCOPE_GUARD([&]() {
    assert(range.start_page != 0);
    assert(range.size != 0);
  })

  // If requested region was already registered
  if (range.size != 0) {
    // Requested region is a sub-region of an existing registration
    if (range.size >= size) return true;

    temp_iterator++;
    // If requested region extends to overlap the following region, erase the
    // entry.
    if (temp_iterator->first < base + size) {
      requested_ranges_.erase(provisional_iterator);
      return false;
    }

    // Get first page of new registration
    uintptr_t new_start_page = GetNextPage(base, range.size);

    // Extend the requested range
    range.size = size;

    // Check for new pages
    if (new_start_page < end_page) {
      // Adjust end of registered page region for overlaps
      uintptr_t new_end_page;
      auto end_block =
          registered_ranges_.find(temp_iterator->second.start_page);
      if (FindContainingBlock(end_page - 1, end_block)) {
        new_end_page = end_block->first;
        end_block->second.Retain();
      } else {
        new_end_page = end_page;
      }

      // Remaining pages
      if (new_start_page < new_end_page) {
        size_t new_length = new_end_page - new_start_page;
        bool status = Runtime::runtime_singleton_->RegisterWithDrivers(
            (void*)new_start_page, new_length);
        assert(status && "KFD registration failure!");
        registered_ranges_[new_start_page] = PageRange(new_length);
      }
    }

    return true;
  }

  // Requested range is new - check for overlaps
  auto before = temp_iterator;
  auto after = temp_iterator;
  // variable: before will be the element before currently inserted element in
  // requested_ranges_, and after will be the element after currently inserted
  // element in requested_ranges_.
  after++;
  before--;
  // If previous requested regions will overlap new region, erase the new entry,
  // and return false
  if (before->first + before->second.size > base) {
    requested_ranges_.erase(provisional_iterator);
    return false;
  }
  // If new requested region overlaps the following requested region, erase the
  // new entry, and return false.
  if (base + size > after->first) {
    requested_ranges_.erase(provisional_iterator);
    return false;
  }
  // Fill out new range
  range.size = size;

  // Get last page block of prior region
  // Start at first block of next region and work back
  auto near_block = registered_ranges_.find(after->second.start_page);
  assert(near_block != registered_ranges_.end() &&
         "Inconsistency in memory database.");

  // Adjust start of registered page region for overlaps
  uintptr_t new_start_page = 0;
  auto start_block = near_block;
  if (FindContainingBlock(start_page, start_block)) {
    range.start_page = start_block->first;
    start_block->second.Retain();
    new_start_page = start_block->first + start_block->second.size_;
  } else {
    new_start_page = start_page;
    range.start_page = start_page;
  }

  // Adjust end of registered page region for overlaps
  uintptr_t new_end_page;
  auto end_block = near_block;
  if (FindContainingBlock(end_page - 1, end_block)) {
    new_end_page = end_block->first;
    // Don't double count a block if the start and end blocks are identical
    if (start_block != end_block) end_block->second.Retain();
  } else {
    new_end_page = end_page;
  }

  // Remaining pages
  // Register new space whose size is equal to new_length with KFD driver and
  // establishs the corresponding mapping in registered_ranges_.
  if (new_start_page < new_end_page) {
    size_t new_length = new_end_page - new_start_page;
    bool ret = Runtime::runtime_singleton_->RegisterWithDrivers(
        (void*)new_start_page, new_length);
    assert(ret && "KFD registration failure!");
    registered_ranges_[new_start_page] = PageRange(new_length);
    if (range.start_page == 0)
      range.start_page = new_start_page;  // When start_page of base is 0, this
                                          // will override the guard element.
  }

  return true;
}

void MemoryDatabase::DeregisterImpl(void* ptr) {
  if (ptr == NULL) return;

  uintptr_t base = (uintptr_t)ptr;

  // Find out if valud of base if an existing key of requested_ranges_, if it
  // is, stores the corresponding iterator to variable of
  // requested_range_iterator.
  auto requested_range_iterator = requested_ranges_.find(base);
  // If ptr is not in requested_ranges_, return.
  if (requested_range_iterator == requested_ranges_.end()) return;

  // Calculate the ending address of the being deleted block and stores it to
  // new variable of end_of_range.
  const uintptr_t end_of_range =
      requested_range_iterator->first + requested_range_iterator->second.size;

  // Find out the corresponding entry in registered_ranges_, and stores the
  // iterator to new variable of registered_range_iterator.
  auto registered_range_iterator =
      registered_ranges_.find(requested_range_iterator->second.start_page);
  assert(registered_range_iterator != registered_ranges_.end() &&
         "Inconsistency in memory database.");

  // Reduce reference counts and release ranges
  while (registered_range_iterator->first < end_of_range) {
    bool release_from_devices = registered_range_iterator->second.Release();
    auto temp = registered_range_iterator;
    temp++;
    if (release_from_devices) {
      Runtime::runtime_singleton_->DeregisterWithDrivers(
          (void*)registered_range_iterator->first);
      registered_ranges_.erase(registered_range_iterator);
    }
    registered_range_iterator = temp;
    assert(registered_range_iterator != registered_ranges_.end() &&
           "Inconsistency in memory database.");
  }

  // Removes the corresponding entry from the requested_ranges_.
  requested_ranges_.erase(requested_range_iterator);
}
}  // namespace core