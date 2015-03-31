cmake_minimum_required(VERSION 2.8)
## GCC 4.8 or higher compiler required.

###############################################################################
## Before building please provide the location for HSA KFD thunk headers and
## .so library files in HSA_BUILD_INC_PATH and HSA_BUILD_LIB_PATH environment
## variables.
## Example:
##    export HSA_BUILD_INC_PATH=<install_dir>/libhsakmt/include
##    export HSA_BUILD_LIB_PATH=<install_dir>/libhsakmt/lib/lnx64
##  
## Obtaining kfd-thunk  headers and shared library:
##
## Headers: Get kfd-thunk sources from git repository:
##          <install_dir>$git clone http://cgit.freedesktop.org/~gabbayo/libhsakmt
##          export HSA_BUILD_INC_PATH=<install_dir>/libhsakmt/include 
##
## Library: Download libhsakmt.so.1 into <install_dir>/libhsakmt/lib/lnx64 from the link below:
##          https://github.com/HSAFoundation/HSA-Drivers-Linux-AMD/tree/master/kfd-0.9/libhsakmt/lnx64a
##          export HSA_BUILD_LIB_PATH=<install_dir>/libhsakmt/lib/lnx64
##          Create a soft link: ln -s libhsakmt.so.1  libhsakmt.so
##
## Now it's ready to build, please see the above "Usage" instructions.
###############################################################################

###############################################################################
## Usage:
## 1) Create build folder named "debug64" (any name will do) under hsa/runtime/core
## 2) Make hsa/runtime/core/debug64 current folder 
## 3) Run "cmake ../ && make"
## libhsa-runtime{64}.so will be in hsa/runtime/core/debug64 folder.
##
## Defaults to 64 bit Debug build.
##    To build Release 32 bit:
##      export HSA_BUILD_TARGET_TYPE=release
##      export HSA_BUILD_TARGET_BITS=32
##    Create another build folder, lets say release32 under hsa/runtime/core
##    and build it from there.
##
## Clean/Rebuild build:
##    * Delete build folder
##    * Follow above 1) 2) 3) steps.
###############################################################################

if(WIN32)
  MESSAGE("-------- Windows build is not supported.")
  RETURN()
endif()

## Process environment variables.
string(TOLOWER "$ENV{HSA_BUILD_TARGET_TYPE}" tmp)
if("${tmp}" STREQUAL release)
  set(BUILD_TYPE "Release")
  set(ISDEBUG 0)
else()
  set(BUILD_TYPE "Debug")
  set(ISDEBUG 1)
endif()

if("$ENV{HSA_BUILD_TARGET_BITS}" STREQUAL 32)
  set(ONLY64STR "")
  set(IS64BIT 0)
else()
  set(ONLY64STR "64")
  set(IS64BIT 1)
endif()

if(NOT EXISTS $ENV{HSA_BUILD_INC_PATH}/hsakmt.h)
  MESSAGE("-------- ERROR: Environment variable HSA_BUILD_INC_PATH is not set to point to the location where KFD Thunk header file hsakmt.h (and rest of the thunk headers) could be found.")
  RETURN()
endif()

if(NOT EXISTS $ENV{HSA_BUILD_LIB_PATH}/libhsakmt.so.1)
  MESSAGE("-------- ERROR: Environment variable HSA_BUILD_LIB_PATH is not set to point to the location where KFD Thunk library libhsakmt.so.1 could be found.")
  RETURN()
endif()

MESSAGE(------IS64BIT: ${IS64BIT})
MESSAGE(------Compiler: ${CMAKE_CXX_COMPILER})
MESSAGE(------Version: ${CMAKE_CXX_COMPILER_VERSION})

## Set core runtuime module name and project name.
set(CORE_RUNTIME_NAME "hsa-runtime${ONLY64STR}")
project (${CORE_RUNTIME_NAME})

set(CMAKE_BUILD_TYPE ${BUILD_TYPE})

## Verbose output.
SET( CMAKE_VERBOSE_MAKEFILE on )

## Compiler preproc definitions.
add_definitions(-D__linux__)
add_definitions(-DLITTLEENDIAN_CPU=1)

## ------------------------- Linux Compiler and Linker options -------------------------
set(CMAKE_CXX_FLAGS "-std=c++11 ")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -fexceptions -fno-rtti -Wno-error=sign-compare -Wno-sign-compare -Wno-write-strings -Wno-deprecated-declarations -Wno-conversion-null -fno-math-errno -fno-threadsafe-statics -fmerge-all-constants -fms-extensions -Wno-error=comment -Wno-comment -Wno-error=pointer-arith -Wno-pointer-arith")

if (IS64BIT) 
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m64  -msse -msse2")
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
endif()

if(ISDEBUG)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ggdb")
endif()

set(CMAKE_EXE_LINKER_FLAGS "-Wl,-Bdynamic -Wl,-z,noexecstack -Wl ")

set(CMAKE_EXE_LINKER_FLAGS "-Wl,-soname=$(CORE_RUNTIME_NAME).so.1 ")
##  -------------------------  End Compiler and Linker options ----------------------------

## Source files.
set (CORE_SRCS util/lnx/os_linux.cpp)
set (CORE_SRCS ${CORE_SRCS} util/small_heap.cpp)
set (CORE_SRCS ${CORE_SRCS} runtime/amd_cpu_agent.cpp)
set (CORE_SRCS ${CORE_SRCS} runtime/amd_gpu_agent.cpp)
set (CORE_SRCS ${CORE_SRCS} runtime/amd_hw_aql_command_processor.cpp)
set (CORE_SRCS ${CORE_SRCS} runtime/amd_memory_region.cpp)
set (CORE_SRCS ${CORE_SRCS} runtime/amd_memory_registration.cpp)
set (CORE_SRCS ${CORE_SRCS} runtime/amd_topology.cpp)
set (CORE_SRCS ${CORE_SRCS} runtime/default_signal.cpp)
set (CORE_SRCS ${CORE_SRCS} runtime/host_queue.cpp)
set (CORE_SRCS ${CORE_SRCS} runtime/hsa.cpp)
set (CORE_SRCS ${CORE_SRCS} runtime/hsa_api_trace.cpp)
set (CORE_SRCS ${CORE_SRCS} runtime/hsa_code_unit.cpp)
set (CORE_SRCS ${CORE_SRCS} runtime/hsa_ext_interface.cpp)
set (CORE_SRCS ${CORE_SRCS} runtime/hsa_ext_amd.cpp)
set (CORE_SRCS ${CORE_SRCS} runtime/interrupt_signal.cpp)
set (CORE_SRCS ${CORE_SRCS} runtime/memory_database.cpp)
set (CORE_SRCS ${CORE_SRCS} runtime/runtime.cpp)

## Include path(s).
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)
include_directories($ENV{HSA_BUILD_INC_PATH})

## Library path(s).
link_directories($ENV{HSA_BUILD_LIB_PATH})

add_library(${CORE_RUNTIME_NAME} SHARED ${CORE_SRCS})

target_link_libraries(${CORE_RUNTIME_NAME} hsakmt elf c stdc++ dl pthread rt)
