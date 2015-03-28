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

// Helpers to use non-atomic types with C++11 atomic operations.

#ifndef HSA_RUNTIME_CORE_UTIL_ATOMIC_HELPERS_H_
#define HSA_RUNTIME_CORE_UTIL_ATOMIC_HELPERS_H_

#include <atomic>
#include "utils.h"

/// @brief: Special assert used here to check each atomic variable for lock free
/// implementation.
/// ANY locked atomics are very likely incompatable with out-of-library
/// concurrent access (HW access for instance)
#define lockless_check(exp) assert(exp)

namespace atomic {
/// @brief: Checks if type T is compatible with its atomic representation.
/// @param: ptr(Input), a pointer to type T for check.
/// @return: void.
template <class T>
static __forceinline void BasicCheck(const T* ptr) {
  static_assert(sizeof(T) == sizeof(std::atomic<T>),
                "Type is size incompatible with its atomic representation!");
  lockless_check(
      reinterpret_cast<const std::atomic<T>*>(ptr)->is_lock_free() &&
      "Atomic operation is not lock free!  Use may conflict with peripheral HW "
      "atomics!");
};

/// @brief: function overloading, for more info, see previous one.
/// @param: ptr(Input), a pointer to a volatile type.
/// @return: void.
template <class T>
static __forceinline void BasicCheck(const volatile T* ptr) {
  static_assert(sizeof(T) == sizeof(std::atomic<T>),
                "Type is size incompatible with its atomic representation!");
  lockless_check(
      reinterpret_cast<const volatile std::atomic<T>*>(ptr)->is_lock_free() &&
      "Atomic operation is not lock free!  Use may conflict with peripheral HW "
      "atomics!");
};

/// @brief: Load value of type T atomically with specified memory order.
/// @param: ptr(Input), a pointer to type T.
/// @param: order(Input), memory order with atomic load, relaxed by default.
/// @return: T, loaded value.
template <class T>
static __forceinline T
    Load(const T* ptr, std::memory_order order = std::memory_order_relaxed) {
  BasicCheck<T>(ptr);
  const std::atomic<T>* aptr = reinterpret_cast<const std::atomic<T>*>(ptr);
  return aptr->load(order);
}

/// @brief: function overloading, for more info, see previous one.
/// @param: ptr(Input), a pointer to volatile type T.
/// @param: order(Input), memory order with atomic load, relaxed by default.
/// @return: T, loaded value.
template <class T>
static __forceinline T
    Load(const volatile T* ptr,
         std::memory_order order = std::memory_order_relaxed) {
  BasicCheck<T>(ptr);
  volatile const std::atomic<T>* aptr =
      reinterpret_cast<volatile const std::atomic<T>*>(ptr);
  return aptr->load(order);
}

/// @brief: Store value of type T with specified memory order.
/// @param: ptr(Input), a pointer to instance which will be stored.
/// @param: val(Input), value to be stored.
/// @param: order(Input), memory order with atomic store, relaxed by default.
/// @return: void.
template <class T>
static __forceinline void Store(
    T* ptr, T val, std::memory_order order = std::memory_order_relaxed) {
  BasicCheck<T>(ptr);
  std::atomic<T>* aptr = reinterpret_cast<std::atomic<T>*>(ptr);
  aptr->store(val, order);
}

/// @brief: Function overloading, for more info, see previous one.
/// @param: ptr(Input), a pointer to volatile instance which will be stored.
/// @param: val(Input), value to be stored.
/// @param: order(Input), memory order with atomic store, relaxed by default.
/// @return: void.
template <class T>
static __forceinline void Store(
    volatile T* ptr, T val,
    std::memory_order order = std::memory_order_relaxed) {
  BasicCheck<T>(ptr);
  volatile std::atomic<T>* aptr =
      reinterpret_cast<volatile std::atomic<T>*>(ptr);
  aptr->store(val, order);
}

/// @brief: Compare and swap value atomically with specified memory order.
/// @param: ptr(Input), a pointer to variable which is operated on.
/// @param: val(Input), value to be stored if condition is satisfied.
/// @param: expected(Input), value which is expected.
/// @param: order(Input), memory order with atomic operation.
/// @return: T, observed value of type T.
template <class T>
static __forceinline T
    Cas(T* ptr, T val, T expected,
        std::memory_order order = std::memory_order_relaxed) {
  BasicCheck<T>(ptr);
  std::atomic<T>* aptr = reinterpret_cast<std::atomic<T>*>(ptr);
  aptr->compare_exchange_strong(expected, val, order);
  return expected;
}

/// @brief: Function overloading, for more info, see previous one.
/// @param: ptr(Input), a pointer to volatile variable which is operated on.
/// @param: val(Input), value to be stored if condition is satisfied.
/// @param: expected(Input), value which is expected.
/// @param: order(Input), memory order which is relaxed by default.
/// @return: T, observed value of type T.
template <class T>
static __forceinline T
    Cas(volatile T* ptr, T val, T expected,
        std::memory_order order = std::memory_order_relaxed) {
  BasicCheck<T>(ptr);
  volatile std::atomic<T>* aptr =
      reinterpret_cast<volatile std::atomic<T>*>(ptr);
  aptr->compare_exchange_strong(expected, val, order);
  return expected;
}

/// @brief: Exchange the value atomically with specified memory order.
/// @param: ptr(Input), a pointer to variable which is operated on.
/// @param: val(Input), value to be stored.
/// @param: order(Input), memory order which is relaxed by default.
/// @return: T, the value prior to the exchange.
template <class T>
static __forceinline T
    Exchange(T* ptr, T val,
             std::memory_order order = std::memory_order_relaxed) {
  BasicCheck<T>(ptr);
  std::atomic<T>* aptr = reinterpret_cast<std::atomic<T>*>(ptr);
  return aptr->exchange(val, order);
}

/// @brief: Function overloading, for more info, see previous one.
/// @param: ptr(Input), a pointer to variable which is operated on.
/// @param: val(Input), value to be stored.
/// @param: order(Input), memory order which is relaxed by default.
/// @return: T, the value prior to the exchange.
template <class T>
static __forceinline T
    Exchange(volatile T* ptr, T val,
             std::memory_order order = std::memory_order_relaxed) {
  BasicCheck<T>(ptr);
  volatile std::atomic<T>* aptr =
      reinterpret_cast<volatile std::atomic<T>*>(ptr);
  return aptr->exchange(val, order);
}

/// @brief: Add value to variable atomically with specified memory order.
/// @param: ptr(Input), a pointer to variable which is operated on.
/// @param: val(Input), value to be added.
/// @param: order(Input), memory order which is relaxed by default.
/// @return: T, the value of the variable prior to the addition.
template <class T>
static __forceinline T
    Add(T* ptr, T val, std::memory_order order = std::memory_order_relaxed) {
  BasicCheck<T>(ptr);
  std::atomic<T>* aptr = reinterpret_cast<std::atomic<T>*>(ptr);
  return aptr->fetch_add(val, order);
}

/// @brief: Subtract value from the variable atomically with specified memory
/// order.
/// @param: ptr(Input), a pointer to variable which is operated on.
/// @param: val(Input), value to be subtraced.
/// @param: order(Input), memory order which is relaxed by default.
/// @return: T, value of the variable prior to the subtraction.
template <class T>
static __forceinline T
    Sub(T* ptr, T val, std::memory_order order = std::memory_order_relaxed) {
  BasicCheck<T>(ptr);
  std::atomic<T>* aptr = reinterpret_cast<std::atomic<T>*>(ptr);
  return aptr->fetch_sub(val, order);
}

/// @brief: Bit And operation on variable atomically with specified memory
/// order.
/// @param: ptr(Input), a pointer to variable which is operated on.
/// @param: val(Input), value which is ANDed with variable.
/// @param: order(Input), memory order which is relaxed by default.
/// @return: T, value of variable prior to the operation.
template <class T>
static __forceinline T
    And(T* ptr, T val, std::memory_order order = std::memory_order_relaxed) {
  BasicCheck<T>(ptr);
  std::atomic<T>* aptr = reinterpret_cast<std::atomic<T>*>(ptr);
  return aptr->fetch_and(val, order);
}

/// @brief: Bit Or operation on variable atomically with specified memory order.
/// @param: ptr(Input), a pointer to variable which is operated on.
/// @param: val(Input), value which is ORed with variable.
/// @param: order(Input), memory order which is relaxed by default.
/// @return: T, value of variable prior to the operation.
template <class T>
static __forceinline T
    Or(T* ptr, T val, std::memory_order order = std::memory_order_relaxed) {
  BasicCheck<T>(ptr);
  std::atomic<T>* aptr = reinterpret_cast<std::atomic<T>*>(ptr);
  return aptr->fetch_or(val, order);
}

/// @brief: Bit Xor operation on variable atomically with specified memory
/// order.
/// @param: ptr(Input), a pointer to variable which is operated on.
/// @param: val(Input), value which is XORed with variable.
/// @order: order(Input), memory order which is relaxed by default.
/// @return: T, valud of variable prior to the opertaion.
template <class T>
static __forceinline T
    Xor(T* ptr, T val, std::memory_order order = std::memory_order_relaxed) {
  BasicCheck<T>(ptr);
  std::atomic<T>* aptr = reinterpret_cast<std::atomic<T>*>(ptr);
  return aptr->fetch_xor(val, order);
}

/// @brief: Increase the value of variable atomically with specified memory
/// order.
/// @param: ptr(Input), a pointer to variable which is operated on.
/// @param: order(Input), memory order which is relaxed by default.
/// @return: T, value of variable prior to the operation.
template <class T>
static __forceinline T
    Increment(T* ptr, std::memory_order order = std::memory_order_relaxed) {
  BasicCheck<T>(ptr);
  std::atomic<T>* aptr = reinterpret_cast<std::atomic<T>*>(ptr);
  return aptr->fetch_add(1, order);
}

/// @brief: Decrease the value of the variable atomically with specified memory
/// order.
/// @param: ptr(Input), a pointer to variable which is operated on.
/// @param: order(Input), memory order which is relaxed by default.
/// @return: T, value of variable prior to the operation.
template <class T>
static __forceinline T
    Decrement(T* ptr, std::memory_order order = std::memory_order_relaxed) {
  BasicCheck<T>(ptr);
  std::atomic<T>* aptr = reinterpret_cast<std::atomic<T>*>(ptr);
  return aptr->fetch_sub(1, order);
}

/// @brief: Add value to variable atomically with specified memory order.
/// @param: ptr(Input), a pointer to volatile variable which is operated on.
/// @param: val(Input), value to be added.
/// @param: order(Input), memory order which is relaxed by default.
/// @return: T, the value of the variable prior to the addition.
template <class T>
static __forceinline T
    Add(volatile T* ptr, T val,
        std::memory_order order = std::memory_order_relaxed) {
  BasicCheck<T>(ptr);
  volatile std::atomic<T>* aptr =
      reinterpret_cast<volatile std::atomic<T>*>(ptr);
  return aptr->fetch_add(val, order);
}

/// @brief: Subtract value from the variable atomically with specified memory
/// order.
/// @param: ptr(Input), a pointer to volatile variable which is operated on.
/// @param: val(Input), value to be subtraced.
/// @param: order(Input), memory order which is relaxed by default.
/// @return: T, value of the variable prior to the subtraction.
template <class T>
static __forceinline T
    Sub(volatile T* ptr, T val,
        std::memory_order order = std::memory_order_relaxed) {
  BasicCheck<T>(ptr);
  volatile std::atomic<T>* aptr =
      reinterpret_cast<volatile std::atomic<T>*>(ptr);
  return aptr->fetch_sub(val, order);
}

/// @brief: Bit And operation on variable atomically with specified memory
/// order.
/// @param: ptr(Input), a pointer to volatile variable which is operated on.
/// @param: val(Input), value which is ANDed with variable.
/// @param: order(Input), memory order which is relaxed by default.
/// @return: T, value of variable prior to the operation.
template <class T>
static __forceinline T
    And(volatile T* ptr, T val,
        std::memory_order order = std::memory_order_relaxed) {
  BasicCheck<T>(ptr);
  volatile std::atomic<T>* aptr =
      reinterpret_cast<volatile std::atomic<T>*>(ptr);
  return aptr->fetch_and(val, order);
}

/// @brief: Bit Or operation on variable atomically with specified memory order.
/// @param: ptr(Input), a pointer to volatile variable which is operated on.
/// @param: val(Input), value which is ORed with variable.
/// @param: order(Input), memory order which is relaxed by default.
/// @return: T, value of variable prior to the operation.
template <class T>
static __forceinline T Or(volatile T* ptr, T val,
                          std::memory_order order = std::memory_order_relaxed) {
  BasicCheck<T>(ptr);
  volatile std::atomic<T>* aptr =
      reinterpret_cast<volatile std::atomic<T>*>(ptr);
  return aptr->fetch_or(val, order);
}

/// @brief: Bit Xor operation on variable atomically with specified memory
/// order.
/// @param: ptr(Input), a pointer to volatile variable which is operated on.
/// @param: val(Input), value which is XORed with variable.
/// @order: order(Input), memory order which is relaxed by default.
/// @return: T, valud of variable prior to the opertaion.
template <class T>
static __forceinline T
    Xor(volatile T* ptr, T val,
        std::memory_order order = std::memory_order_relaxed) {
  BasicCheck<T>(ptr);
  volatile std::atomic<T>* aptr =
      reinterpret_cast<volatile std::atomic<T>*>(ptr);
  return aptr->fetch_xor(val, order);
}

/// @brief: Increase the value of variable atomically with specified memory
/// order.
/// @param: ptr(Input), a pointer to volatile variable which is operated on.
/// @param: order(Input), memory order which is relaxed by default.
/// @return: T, value of variable prior to the operation.
template <class T>
static __forceinline T
    Increment(volatile T* ptr,
              std::memory_order order = std::memory_order_relaxed) {
  BasicCheck<T>(ptr);
  volatile std::atomic<T>* aptr =
      reinterpret_cast<volatile std::atomic<T>*>(ptr);
  return aptr->fetch_add(1, order);
}

/// @brief: Decrease the value of the variable atomically with specified memory
/// order.
/// @param: ptr(Input), a pointer to volatile variable which is operated on.
/// @param: order(Input), memory order which is relaxed by default.
/// @return: T, value of variable prior to the operation.
template <class T>
static __forceinline T
    Decrement(volatile T* ptr,
              std::memory_order order = std::memory_order_relaxed) {
  BasicCheck<T>(ptr);
  volatile std::atomic<T>* aptr =
      reinterpret_cast<volatile std::atomic<T>*>(ptr);
  return aptr->fetch_sub(1, order);
}
}

// Remove special assert to avoid name polution
#undef lockless_check

#endif  // HSA_RUNTIME_CORE_UTIL_ATOMIC_HELPERS_H_
