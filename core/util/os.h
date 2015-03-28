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

// Minimal operating system abstraction interfaces.

#ifndef HSA_RUNTIME_CORE_UTIL_OS_H_
#define HSA_RUNTIME_CORE_UTIL_OS_H_

#include <string>
#include "utils.h"

namespace os {
typedef void* LibHandle;
typedef void* Mutex;
typedef void* Thread;
typedef void* EventHandle;

enum class os_t { OS_WIN = 0, OS_LINUX, COUNT };
static __forceinline std::underlying_type<os_t>::type os_index(os_t val) {
  return std::underlying_type<os_t>::type(val);
}

#ifdef _WIN32
static const os_t current_os = os_t::OS_WIN;
#elif __linux__
static const os_t current_os = os_t::OS_LINUX;
#else
static_assert(false, "Operating System not detected!");
#endif

/// @brief: Loads dynamic library based on file name. Return value will be NULL
/// if failed.
/// @param: filename(Input), file name of the library.
/// @return: LibHandle.
LibHandle LoadLib(std::string filename);

/// @brief: Gets the address of exported symbol. Return NULl if failed.
/// @param: lib(Input), library handle which exporting from.
/// @param: export_name(Input), the name of the exported symbol.
/// @return: void*.
void* GetExportAddress(LibHandle lib, std::string export_name);

/// @brief: Unloads the dynamic library.
/// @param: lib(Input), library handle which will be unloaded.
void CloseLib(LibHandle lib);

/// @brief: Creates a mutex, will return NULL if failed.
/// @param: void.
/// @return: Mutex.
Mutex CreateMutex();

/// @brief: Tries to acquire the mutex once, if successed, return true.
/// @param: lock(Input), handle to the mutex.
/// @return: bool.
bool TryAcquireMutex(Mutex lock);

/// @brief: Aquires the mutex, if the mutex is locked, it will wait until it is
/// released. If the mutex is acquired successfully, it will return true.
/// @param: lock(Input), handle to the mutex.
/// @return: bool.
bool AcquireMutex(Mutex lock);

/// @brief: Releases the mutex.
/// @param: lock(Input), handle to the mutex.
/// @return: void.
void ReleaseMutex(Mutex lock);

/// @brief: Destroys the mutex.
/// @param: lock(Input), handle to the mutex.
/// @return: void.
void DestroyMutex(Mutex lock);

/// @brief: Puts current thread to sleep.
/// @param: delayInMs(Input), time in millisecond for sleeping.
/// @return: void.
void Sleep(int delayInMs);

/// @brief: Yields current thread.
/// @param: void.
/// @return: void.
void YieldThread();

typedef void (*ThreadEntry)(void*);

/// @brief: Creates a thread will return NULL if failed.
/// @param: entry_function(Input), a pointer to the function which the thread
/// starts from.
/// @param: entry_argument(Input), a pointer to the argument of the thread
/// function.
/// @param: stack_size(Input), size of the thread's stack, 0 by default.
/// @return: Thread, a handle to thread created.
Thread CreateThread(ThreadEntry entry_function, void* entry_argument,
                    uint stack_size = 0);

/// @brief: Destroys the thread.
/// @param: thread(Input), thread handle to what will be destroyed.
/// @return: void.
void CloseThread(Thread thread);

/// @brief: Waits for specific thread to finish, if successed, return true.
/// @param: thread(Input), handle to waiting thread.
/// @return: bool.
bool WaitForThread(Thread thread);

/// @brief: Waits for multiple threads to finish, if successed, return ture.
/// @param; threads(Input), a pointer to a list of thread handle.
/// @param: thread_count(Input), number of threads to be waited on.
/// @return: bool.
bool WaitForAllThreads(Thread* threads, uint thread_count);

/// @brief: Sets the environment value.
/// @param: env_var_name(Input), name of the environment value.
/// @param: env_var_value(Input), value of the environment value.s
/// @return: void.
void SetEnvVar(std::string env_var_name, std::string env_var_value);

/// @brief: Gets the value of environment value.
/// @param: env_var_name(Input), name of the environment value.
/// @return: std::string, value of the environment value, returned as string.
std::string GetEnvVar(std::string env_var_name);

/// @brief: Gets the max virtual memory size accessible to the application.
/// @param: void.
/// @return: size_t, size of the accessible memory to the application.
size_t GetUserModeVirtualMemorySize();

/// @brief: Gets the virtual memory base address. It is hardcoded to 0.
/// @param: void.
/// @return: uintptr_t, always 0.
uintptr_t GetUserModeVirtualMemoryBase();

/// @brief os event api, create an event
/// @param: auto_reset whether an event can reset the status automatically
/// @param: init_state initial state of the event
/// @return: event handle
EventHandle CreateOsEvent(bool auto_reset, bool init_state);

/// @brief os event api, destroy an event
/// @param: event handle
/// @return: whether destroy is correct
int DestroyOsEvent(EventHandle event);

/// @brief os event api, wait on event
/// @param: event Event handle
/// @param: milli_seconds wait time
/// @return: Indicate success or timeout 
int WaitForOsEvent(EventHandle event, unsigned int milli_seconds);

/// @brief os event api, set event state
/// @param: event Event handle
/// @return: Whether event set is correct
int SetOsEvent(EventHandle event);

/// @brief os event api, reset event state
/// @param: event Event handle
/// @return: Whether event reset is correct
int ResetOsEvent(EventHandle event);

}

#endif  // HSA_RUNTIME_CORE_UTIL_OS_H_
