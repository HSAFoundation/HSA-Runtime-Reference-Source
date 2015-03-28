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

#ifdef __linux__

#include <string>

#include "core/util/os.h"
#include <dlfcn.h>
#include "unistd.h"
#include "sched.h"
#include <pthread.h>
#include <sys/time.h>

namespace os {

static_assert(sizeof(LibHandle) == sizeof(void*),
              "OS abstraction size mismatch");
static_assert(sizeof(Mutex) == sizeof(pthread_mutex_t*),
              "OS abstraction size mismatch");
static_assert(sizeof(Thread) == sizeof(pthread_t),
              "OS abstraction size mismatch");

LibHandle LoadLib(std::string filename) {
  void* ret = dlopen(filename.c_str(), RTLD_LAZY);
  return *(LibHandle*)&ret;
}

void* GetExportAddress(LibHandle lib, std::string export_name) {
  return dlsym(*(void**)&lib, export_name.c_str());
}

void CloseLib(LibHandle lib) { dlclose(*(void**)&lib); }

Mutex CreateMutex() {
  pthread_mutex_t* mutex = new pthread_mutex_t;
  pthread_mutex_init(mutex, NULL);
  return *(Mutex*)&mutex;
}

bool TryAcquireMutex(Mutex lock) {
  return pthread_mutex_trylock(*(pthread_mutex_t**)&lock) == 0;
}

bool AcquireMutex(Mutex lock) {
  return pthread_mutex_lock(*(pthread_mutex_t**)&lock) == 0;
}

void ReleaseMutex(Mutex lock) {
  pthread_mutex_unlock(*(pthread_mutex_t**)&lock);
}

void DestroyMutex(Mutex lock) {
  pthread_mutex_destroy(*(pthread_mutex_t**)&lock);
  delete *(pthread_mutex_t**)&lock;
}

void Sleep(int delay_in_millisec) { usleep(delay_in_millisec * 1000); }

void YieldThread() { sched_yield(); }

struct ThreadArgs {
  void* entry_args;
  ThreadEntry entry_function;
};

void* __stdcall ThreadTrampoline(void* arg) {
  ThreadArgs* ar = (ThreadArgs*)arg;
  ThreadEntry CallMe = ar->entry_function;
  void* Data = ar->entry_args;
  delete ar;
  CallMe(Data);
  return NULL;
}

Thread CreateThread(ThreadEntry function, void* threadArgument,
                    uint stackSize) {
  ThreadArgs* args = new ThreadArgs;
  args->entry_args = threadArgument;
  args->entry_function = function;
  pthread_t thread;
  pthread_attr_t attrib;
  pthread_attr_init(&attrib);
  if (stackSize != 0) pthread_attr_setstacksize(&attrib, stackSize);
  bool success =
      (pthread_create(&thread, &attrib, ThreadTrampoline, args) == 0);
  pthread_attr_destroy(&attrib);
  if (!success) {
    pthread_join(thread, NULL);
    return NULL;
  }
  return *(Thread*)&thread;
}

void CloseThread(Thread thread) { pthread_detach(*(pthread_t*)&thread); }

bool WaitForThread(Thread thread) {
  return pthread_join(*(pthread_t*)&thread, NULL);
}

bool WaitForAllThreads(Thread* threads, uint threadCount) {
  for (uint i = 0; i < threadCount; i++) WaitForThread(threads[i]);
  return true;
}

void SetEnvVar(std::string env_var_name, std::string env_var_value) {
  setenv(env_var_name.c_str(), env_var_value.c_str(), 1);
}

std::string GetEnvVar(std::string env_var_name) {
  char* buff;
  buff = getenv(env_var_name.c_str());
  std::string ret;
  if (buff) {
    ret = buff;
  }
  return ret;
}

size_t GetUserModeVirtualMemorySize() {
#ifdef _LP64
  // https://www.kernel.org/doc/Documentation/x86/x86_64/mm.txt :
  // user space is 0000000000000000 - 00007fffffffffff (=47 bits)
  return (size_t)(0x800000000000);
#else
  return (size_t)(0xffffffff);  // ~4GB
#endif
}

uintptr_t GetUserModeVirtualMemoryBase() { return (uintptr_t)0; }

// Os event implementation
typedef struct EventDescriptor_ {
  pthread_cond_t event;
  pthread_mutex_t mutex;
  bool state;
  bool auto_reset;
} EventDescriptor;

EventHandle CreateOsEvent(bool auto_reset, bool init_state) 
{
  EventDescriptor *eventDescrp;
  eventDescrp = (EventDescriptor *)malloc(sizeof(EventDescriptor));

  pthread_mutex_init(&eventDescrp->mutex, NULL);
  pthread_cond_init(&eventDescrp->event, NULL);
  eventDescrp->auto_reset = auto_reset;
  eventDescrp->state = init_state;
  
  EventHandle handle = reinterpret_cast<EventHandle>(eventDescrp);

  return handle;
}

int DestroyOsEvent(EventHandle event)
{
  if (event == NULL) {
    return -1;
  }

  EventDescriptor *eventDescrp = reinterpret_cast<EventDescriptor *>(event);
  int ret_code =  pthread_cond_destroy(&eventDescrp->event);
  ret_code |= pthread_mutex_destroy(&eventDescrp->mutex);
  free(eventDescrp);
  return ret_code;
}

int WaitForOsEvent(EventHandle event, unsigned int milli_seconds)
{
  if (event == NULL) {
    return -1;
  }

  EventDescriptor *eventDescrp = reinterpret_cast<EventDescriptor *>(event);
  // Event wait time is 0 and state is non-signaled, return directly
  if (milli_seconds == 0) {
    int tmp_ret = pthread_mutex_trylock(&eventDescrp->mutex);
      if (tmp_ret == EBUSY) {
        // Timeout
        return 1;
      }
  }

  int ret_code = 0;
  pthread_mutex_lock(&eventDescrp->mutex);
  if (!eventDescrp->state) {
    if (milli_seconds == 0) {
      ret_code = 1;
    }
    else {
      struct timespec ts;
      struct timeval tp;
        
      ret_code = gettimeofday(&tp, NULL);
      ts.tv_sec = tp.tv_sec;
      ts.tv_nsec = tp.tv_usec * 1000;
        
      unsigned int sec = milli_seconds / 1000;
      unsigned int mSec = milli_seconds % 1000;
        
      ts.tv_sec += sec;
      ts.tv_nsec += mSec * 1000000;
        
      // More then one second, add 1 sec to the tv_sec elem
      if (ts.tv_nsec > 1000000000) {
        ts.tv_sec += 1;
        ts.tv_nsec = ts.tv_nsec - 1000000000;
      }
        
      ret_code = pthread_cond_timedwait(&eventDescrp->event, &eventDescrp->mutex, &ts);
      // Time out
      if (ret_code == 110) {
        ret_code = 0x14003; // 1 means time out in HSA
      }

      if (ret_code == 0 && eventDescrp->auto_reset) {
        eventDescrp->state = false;
      }
    }
  }
  else if (eventDescrp->auto_reset) {
    eventDescrp->state = false;
  }
  pthread_mutex_unlock(&eventDescrp->mutex);

  return ret_code;
}

int SetOsEvent(EventHandle event)
{
  if (event == NULL) {
    return -1;
  }

  EventDescriptor *eventDescrp = reinterpret_cast<EventDescriptor *>(event);
  int ret_code = 0;
  ret_code = pthread_mutex_lock(&eventDescrp->mutex);
  eventDescrp->state = true;
  ret_code = pthread_mutex_unlock(&eventDescrp->mutex);
  ret_code |= pthread_cond_signal(&eventDescrp->event);
   
  return ret_code;
}

int ResetOsEvent(EventHandle event) {
  if (event == NULL) {
    return -1;
  }

  EventDescriptor *eventDescrp = reinterpret_cast<EventDescriptor *>(event);
  int ret_code = 0;
  ret_code = pthread_mutex_lock(&eventDescrp->mutex);
  eventDescrp->state = false;
  ret_code = pthread_mutex_unlock(&eventDescrp->mutex);

  return ret_code;
}

}

#endif
