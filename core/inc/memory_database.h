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

// memory_database.h
// Header file of MemoryDatabase class

#ifndef HSA_RUNTIME_CORE_INC_MEMORYDATABASE_H_
#define HSA_RUNTIME_CORE_INC_MEMORYDATABASE_H_

#include "stdint.h"

#include <map>

#include "core/util/utils.h"
#include "core/util/locks.h"

namespace core {

/// @Brief: This is a class to store the information of requested memory regions
/// to driver, to store and register the requested regions from driver if there
/// is no overlap between newly requested and existing regions.
class MemoryDatabase {
 public:
  /// @Variable: kPageSize_, refers to the size in bytes of each page.
  static const size_t kPageSize_ = 4096;

  /// @Brief: Default constructor, will call member function Init simply.
  /// See description for member function Init().
  MemoryDatabase() { Init(); }

  /// @Brief: Get the beginning address of the page which ptr belongs to.
  /// Basically, it sets the last 12 bits of ptr to 0.
  /// @Param: ptr(Input), specify the address for calculation.
  /// @Return: uintptr_t.
  static __forceinline uintptr_t GetPage(uintptr_t ptr) {
    return ptr & (~(kPageSize_ - 1));
  }

  /// @Brief: Get the beginning address of the page which is immediately after
  /// the one which contains ptr.
  /// @Param: ptr(Input), specify the address for calculation.
  /// @Return: uintptr_t.
  static __forceinline uintptr_t GetNextPage(uintptr_t ptr) {
    return GetPage(ptr) + kPageSize_;
  }

  /// @Brief: Get the beginning address of the page which is immediately after
  /// the whole requested block.
  /// @Param: ptr(Input), specify the initial address of the requested block.
  /// @Param: size(Input), specify the size of the requested block.
  static __forceinline uintptr_t GetNextPage(uintptr_t ptr, size_t size) {
    return GetPage(ptr + size - 1) + kPageSize_;
  }

  /// @Brief: Register the requested region from driver and updates the entry in
  /// member variable requested_ranges_ and registered_ranges_ to stores the
  /// information. It is user's responbility to deregister the memory from
  /// driver by explicitly calling member function of Deregister() or
  /// DeregisterAll(). This function simply acquires the lock and call
  // RegisterImpl(), So, it is thread-safe. For more info, refer to function
  /// RegisterImpl.
  ///
  /// @Param: ptr(Input), specify the start address of the requested block, if
  /// ptr is a null pointer, the function does nothing and return false.
  /// @Param: size(Input), specifies the size of the requested block, if size is
  /// less than or equal to zero, the function does nothing and return false.
  /// @Return: bool
  bool Register(void* ptr, size_t size) {
    ScopedAcquire<KernelMutex> lock(&lock_);
    return RegisterImpl(ptr, size);
  }

  /// @Brief: Simply acquire the lock and call DeregisterImpl(), for more info,
  /// see DeregisterImpl(void* ptr). This function is thread-safe.
  /// @Param: ptr(Input), specify the start address of being deregisterd block.
  /// @Return: void.
  void Deregister(void* ptr) {
    ScopedAcquire<KernelMutex> lock(&lock_);
    DeregisterImpl(ptr);
  }

  /// @Brief: Deregister all the memory regions which are registered before and
  /// re-initialize the object again. It will acquire the lock first, so it is
  /// thread-safe.
  /// @Param: void.
  /// @Return: void.
  void DeregisterAll() {
    ScopedAcquire<KernelMutex> lock(&lock_);
    // Remove guard entries
    requested_ranges_.erase(0);
    requested_ranges_.erase(UINTPTR_MAX);

    // Unregister remaining ranges
    while (requested_ranges_.begin() != requested_ranges_.end())
      DeregisterImpl(reinterpret_cast<void*>(requested_ranges_.begin()->first));

    // Remove guard entries from page block map
    registered_ranges_.clear();

    // Reinitialize
    Init();
  }

  /// @Brief: Overload operator "new".
  /// @Param: size(Input), specify the memory size being allocated.
  /// @Return: void*.
  void* operator new(size_t size) { return malloc(size); }

  /// @Brief: Overload operator "new".
  /// @Param: size(Input).
  /// @Param: ptr(Input), a void pointer.
  /// @Return: void*, simply return the value of ptr.
  void* operator new(size_t size, void* ptr) { return ptr; }

  /// @Brief: Overload operator "delete".
  /// @Param: ptr(Input), pointer which points to the memory block which needs
  /// to be deleted.
  /// @Return: void.
  void operator delete(void* ptr) { free(ptr); }

  /// @Brief: Overload operator "delete".
  /// @Param: void*(Input), void*(Input).
  /// @Return: void.
  void operator delete(void*, void*) {}

 private:

  /// @Brief: Used as "value" in a map to store registerd region size and
  /// reference count.
  class PageRange {
   public:
    /// @Brief: default Constructor, simply sets all member variables to 0.
    PageRange() : size_(0), reference_count_(0) {}

    /// @Brief: Constructor to set size_ with parameter and set reference_count_
    /// to 1.
    /// @Param: size(Input), size of the newly registerd memory block.
    explicit PageRange(size_t size) : size_(size), reference_count_(1) {}

    /// @Brief: Decrease use count and finally if it is 0, return ture.
    /// @Param: void.
    /// @Return: bool.
    bool Release() {
      reference_count_--;
      return reference_count_ == 0;
    }

    /// @Brief: Simply increase reference_count_.
    /// @Param: void.
    /// @Return: void.
    void Retain() { reference_count_++; }

    /// @Variable: size_, specify the size in bytes of registerd region from
    /// driver(using bytes because there may be a mix of page sizes).
    size_t size_;

    /// @Variable: reference count for this block of pages
    size_t reference_count_;
  };

  /// @Brief: a structure used as "value" in a map to store requested region
  /// size and its registerd start page address.
  struct Range {
    /// @Brief: Default Constructor, simpley set all member variables to 0.
    Range() : size(0), start_page(0) {}

    /// @Brief: Constructor used to explicitly set all member variables.
    /// @Param: range_size(Input), specify the size of requested region.
    /// @Param: first_page(Input), specify the address of first page of the
    // requested region.
    Range(size_t range_size, uintptr_t first_page)
        : size(range_size), start_page(first_page) {}

    /// @Variable: size of requested region in bytes
    size_t size;
    /// @Variable: start_page, specify the address of first page in overlapped
    /// page blocks (may be prior to the first page of the requested region).
    uintptr_t start_page;
  };

  /// @Brief: Initialize the object content of requested_ranges_and
  /// registered_ranges_. Simply add guard element for two variables.
  /// @Param: void.
  /// @Return: void.
  void Init() {
    /// Ensure that there is a prior and a post region for all requests.
    registered_ranges_[0] = PageRange(1);
    registered_ranges_[UINTPTR_MAX] = PageRange(0);

    requested_ranges_[0] = Range(1, 0);
    requested_ranges_[UINTPTR_MAX] = Range(0, UINTPTR_MAX);
  }

  /// @Brief: Find if input of address resides in a registered region. If found,
  /// return true, and near_hint will point to the element which includes it in
  /// registerd_ranges_.
  /// This funtion will perform much faster if near_hint is close to the desired
  /// block at the start.
  /// @Param: address, specify the address you want to search.
  /// @Param: near_hint, a refernce of a interator which is a hint for search.
  /// @Return: bool.
  bool FindContainingBlock(uintptr_t address,
                           std::map<uintptr_t, PageRange>::iterator& near_hint);

  /// @Brief: Register the requested region from driver and updates the entry in
  /// member variable requested_ranges_ and registered_ranges_ to stores the
  /// information.
  /// It will return ture if requested region is registerd with driver
  /// successfully or it is a sub-region of existing registered region, if
  /// requested region block overlaps with existing registerd region, the
  /// request is abandoned, the entry in requested_ranges_ whose "key" is equal
  /// to the value of ptr will be erased and function returns false. Be careful
  /// if you want to request a block which will overlap other regions, this may
  /// lead to delete the existing entry in requested_ranges_.
  ////
  /// @Param: ptr(Input), specify the start address of the requested block, if
  /// ptr is a null pointer, the function does nothing and return false.
  /// @Param: size(Input), specifies the size of the requested block, if size is
  /// less than or equal to zero, the function does nothing and return false.
  /// @Return: bool
  bool RegisterImpl(void* ptr, size_t size);

  /// @Brief: Deregister memory block from driver and updates the related
  /// informations stored in requested_ranges_ and registered_ranges_.
  /// @Param: ptr(Input), specify the start address of being deregisterd block.
  /// If ptr is null pointer or a value not equal to one of keys in
  /// requsted_ranges_, the function does nothing and return.
  /// @Return: void.
  void DeregisterImpl(void* ptr);

  std::map<uintptr_t, Range> requested_ranges_;
  std::map<uintptr_t, PageRange> registered_ranges_;
  KernelMutex lock_;

  DISALLOW_COPY_AND_ASSIGN(MemoryDatabase);
};
}  // namespace core
#endif  // HSA_RUNTIME_CORE_INC_MEMORYDATABASE_H_
