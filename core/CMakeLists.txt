cmake_minimum_required ( VERSION 2.8.0 )
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
## Library: Go to the libhsakmt folder you cloned in the previous step
##          Run 'make'
##          for 64 bit, export HSA_BUILD_LIB_PATH=
##                                <install_dir>/libhsakmt/build/lnx64a
##          for 32 bit, export HSA_BUILD_LIB_PATH=
##                                <install_dir>/libhsakmt/build/lnx
##
## The system environment is now ready to build HSA. Please see the
## "Usage" instructions below.

###############################################################################
## Usage:
## 1) Create build folder named "release64" (any name will do) under core
## 2) Make core/release64 current folder
## 3) Run "cmake ../ && make"
## libhsa-runtime{64}.so will be in core/release64 folder.
##
## Defaults to 64 bit Release build.
##    To build 32 bit:
##      export HSA_BUILD_TARGET_BITS=32
##
## Note: The CMAKE_BUILD_TYPE CMake variable will override the
##       HSA_BUILD_TARGET_TYPE environment variable. Its value
##       can be specified on the cmake command line using the
##       "-D CMAKE_BUILD_TYPE:STRING=(Release|Debug)" option.

if ( WIN32 )
    MESSAGE ( FATAL_ERROR "Windows build is not supported." )
endif ()

if ( "$ENV{HSA_BUILD_TARGET_BITS}" STREQUAL 32 )
    set ( ONLY64STR "" )
    set ( IS64BIT 0 )
else ()
    set ( ONLY64STR "64" )
    set ( IS64BIT 1 )
endif ()

if ( NOT EXISTS $ENV{HSA_BUILD_INC_PATH}/hsakmt.h )
    MESSAGE ( FATAL_ERROR "Environment variable HSA_BUILD_INC_PATH is not set to point to the location where KFD Thunk header file hsakmt.h (and rest of the thunk headers) could be found." )
endif ()

if ( NOT EXISTS $ENV{HSA_BUILD_LIB_PATH}/libhsakmt.so.1 )
    MESSAGE ( FATAL_ERROR "Environment variable HSA_BUILD_LIB_PATH is not set to point to the location where KFD Thunk library libhsakmt.so.1 could be found." )
endif ()

MESSAGE ( ------IS64BIT: ${IS64BIT} )
MESSAGE ( ------Compiler: ${CMAKE_CXX_COMPILER} )
MESSAGE ( ------Version: ${CMAKE_CXX_COMPILER_VERSION} )

## Set core runtuime module name and project name.
set ( CORE_RUNTIME_PACKAGE "hsa-runtime" )
set ( CORE_RUNTIME_LIB "${CORE_RUNTIME_PACKAGE}${ONLY64STR}" )
project ( ${CORE_RUNTIME_LIB} )

## Verbose output.
set ( CMAKE_VERBOSE_MAKEFILE on )

## Compiler preproc definitions.
add_definitions ( -D__linux__ )
add_definitions ( -DLITTLEENDIAN_CPU=1 )
add_definitions ( -DHSA_EXPORT=1 )
add_definitions ( -DHSA_EXPORT_FINALIZER=1 )
add_definitions ( -DHSA_EXPORT_IMAGES=1 )

## ------------------------- Linux Compiler and Linker options -------------------------
set ( CMAKE_CXX_FLAGS "-std=c++11 " )

set ( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -fexceptions -fno-rtti -fvisibility=hidden -Wno-error=sign-compare -Wno-sign-compare -Wno-write-strings -Wno-deprecated-declarations -Wno-conversion-null -fno-math-errno -fno-threadsafe-statics -fmerge-all-constants -fms-extensions -Wno-error=comment -Wno-comment -Wno-error=pointer-arith -Wno-pointer-arith" )

if ( IS64BIT )
    set ( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m64 -msse -msse2" )
else ()
    set ( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32" )
endif ()

set ( DRVDEF "${CMAKE_SOURCE_DIR}/hsacore.so.def" )

set ( CMAKE_SHARED_LINKER_FLAGS "-Wl,-Bdynamic -Wl,-z,noexecstack -Wl,--version-script=${DRVDEF}" )

set ( CMAKE_SKIP_BUILD_RPATH TRUE)

##  -------------------------  End Compiler and Linker options ----------------------------

## Source files.
set ( CORE_SRCS util/lnx/os_linux.cpp )
set ( CORE_SRCS ${CORE_SRCS} util/small_heap.cpp )
set ( CORE_SRCS ${CORE_SRCS} util/timer.cpp )
set ( CORE_SRCS ${CORE_SRCS} runtime/amd_blit_kernel.cpp )
set ( CORE_SRCS ${CORE_SRCS} runtime/amd_blit_sdma.cpp )
set ( CORE_SRCS ${CORE_SRCS} runtime/amd_cpu_agent.cpp )
set ( CORE_SRCS ${CORE_SRCS} runtime/amd_gpu_agent.cpp )
set ( CORE_SRCS ${CORE_SRCS} runtime/amd_dgpu_agent.cpp )
set ( CORE_SRCS ${CORE_SRCS} runtime/amd_hw_aql_command_processor.cpp )
set ( CORE_SRCS ${CORE_SRCS} runtime/amd_memory_region.cpp )
set ( CORE_SRCS ${CORE_SRCS} runtime/amd_memory_registration.cpp )
set ( CORE_SRCS ${CORE_SRCS} runtime/amd_sdma_cmdwriter_kv.cpp )
set ( CORE_SRCS ${CORE_SRCS} runtime/amd_topology.cpp )
set ( CORE_SRCS ${CORE_SRCS} runtime/compute_capability.cpp )
set ( CORE_SRCS ${CORE_SRCS} runtime/default_signal.cpp )
set ( CORE_SRCS ${CORE_SRCS} runtime/host_queue.cpp )
set ( CORE_SRCS ${CORE_SRCS} runtime/hsa.cpp )
set ( CORE_SRCS ${CORE_SRCS} runtime/hsa_api_trace.cpp )
set ( CORE_SRCS ${CORE_SRCS} runtime/hsa_ext_amd.cpp )
set ( CORE_SRCS ${CORE_SRCS} runtime/hsa_ext_interface.cpp )
set ( CORE_SRCS ${CORE_SRCS} runtime/interrupt_signal.cpp )
set ( CORE_SRCS ${CORE_SRCS} runtime/isa.cpp )
set ( CORE_SRCS ${CORE_SRCS} runtime/amd_loader_context.cpp )
set ( CORE_SRCS ${CORE_SRCS} runtime/amd_load_map.cpp )
set ( CORE_SRCS ${CORE_SRCS} runtime/memory_database.cpp )
set ( CORE_SRCS ${CORE_SRCS} runtime/runtime.cpp )
set ( CORE_SRCS ${CORE_SRCS} runtime/signal.cpp )
set ( CORE_SRCS ${CORE_SRCS} common/hsa_table_interface.cpp )
set ( CORE_SRCS ${CORE_SRCS} common/shared.cpp )
set ( CORE_SRCS ${CORE_SRCS} tools/libamdhsacode/amd_elf_image.cpp )
set ( CORE_SRCS ${CORE_SRCS} tools/libamdhsacode/amd_hsa_code.cpp )
set ( CORE_SRCS ${CORE_SRCS} tools/libamdhsacode/amd_hsa_code_util.cpp )
set ( CORE_SRCS ${CORE_SRCS} tools/libamdhsacode/amd_hsa_locks.cpp )
set ( CORE_SRCS ${CORE_SRCS} tools/libamdhsacode/util.cpp )
set ( CORE_SRCS ${CORE_SRCS} tools/libamdhsacode/xutil.cpp )
set ( CORE_SRCS ${CORE_SRCS} tools/loader/executable.cpp )
 
## Include path(s).
include_directories ( ${CMAKE_CURRENT_SOURCE_DIR}/.. )
include_directories ( ${CMAKE_SOURCE_DIR}/../inc )
include_directories ( ${CMAKE_SOURCE_DIR}/inc )
include_directories ( ${CMAKE_SOURCE_DIR}/tools/libamdhsacode )
include_directories ( ${CMAKE_SOURCE_DIR}/tools/loader )
include_directories ( $ENV{HSA_BUILD_INC_PATH} )

## Library path(s).
link_directories ( $ENV{HSA_BUILD_LIB_PATH} )

add_library ( ${CORE_RUNTIME_LIB} SHARED ${CORE_SRCS} )

set_property ( TARGET ${CORE_RUNTIME_LIB} PROPERTY SOVERSION 1 )

target_link_libraries ( ${CORE_RUNTIME_LIB} hsakmt elf c stdc++ dl pthread rt )
