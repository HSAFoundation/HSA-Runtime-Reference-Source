HSA-Runtime-Reference-Source
============================

This repository contains the HSA Runtime source code, released primarily for user reference and experimentation purposes.

### Package Contents

##### Source & Include directories

core - Contains the source code for AMD's implementation of the core HSA Runtime API's.

inc - Contains the public and AMD specific header files exposing the HSA Runtimes interfaces.

### Build environment

CMake build framework is used to build the HSA runtime. The minimum version is 2.8.

Obtain cmake infrastructure: http://www.cmake.org/download/

Export cmake bin into your PATH
HSA Runtime CMake build file CMakeLists.txt is located in runtime/core folder.

Refer to CMakeLists.txt for building instructions and setup
The 32-bit build target is a shared library named libhsa-runtime.so
The 64-bit target is libhsa-runtime64.so

### Package Dependencies

The following support packages are requried to succesfully build the runtime:

* libelf-dev
* g++
* libc6-dev-i386 (for libhsakmt 32bit)

### Specs

http://www.hsafoundation.com/standards/

HSA Runtime Specification 1.0

HSA Programmer Reference Manual Specification 1.0

HSA Platform System Architecture Specification 1.0

### Runtime Design overview

The AMD HSA runtime consists of three primary layers:

C interface adaptors
C++ interfaces classes and common functions
AMD device specific implementations
Additionally the runtime is dependent on a small utility library which provides simple common functions, limited operating system and compiler abstraction, as well as atomic operation interfaces.

#### C interface adaptors

Files :

hsa.h(cpp)

hsa_ext_interface.h(cpp)

The C interface layer provides C99 APIs as defined in the HSA Runtime Specification 1.0 Provisional. The interfaces and default definitions for the standard extensions are also provided. The interface functions simply forward to a function pointer table defined here. The table is initialized to point to default definitions, which simply return an appropriate error code. In this release the standard extensions (image support and finalizer) are implemented in a separate library called \vendors\amd\lib\lihsa-runtime-amd-ext[64].so. If available the extension library is loaded as part of runtime initialization and the table is updated to point into the extension library.

#### C++ interfaces classes and common functions

Files :

runtime.h(cpp)

agent.h

queue.h

signal.h

memory_region.h(cpp)

checked.h

memory_database.h(cpp)

default_signal.h(cpp)

The C++ interface layer provides abstract interface classes encapsulating commands to HSA Signals, Agents, and Queues. This layer also contains the implementation of device independent commands, such as hsa_init and hsa_system_get_info, and a default signal and queue implementation.

#### Device Specific Implementations

Files:

amd_cpu_agent.h(cpp)

amd_gpu_agent.h(cpp)

amd_hw_aql_command_processor.h(cpp)

amd_memory_region.h(cpp)

amd_memory_registration.h(cpp)

amd_topology.h(cpp)

host_queue.h(cpp)

interrupt_signal.h(cpp)

hsa_ext_private_amd.h(cpp)

The device specific layer contains implementations of the C++ interface classes which implement HSA functionality for AMD 7000 series APUs.

#### Unimplemented functionality

* The following queries are not implemented:
<<<<<<< HEAD
  ** hsa_code_symbol_get_info: HSA_CODE_SYMBOL_INFO_VARIABLE_ALIGNMENT, HSA_CODE_SYMBOL_INFO_INDIRECT_FUNCTION_CALL_CONVENTION
  ** hsa_executable_symbol_get_info: HSA_EXECUTABLE_SYMBOL_INFO_VARIABLE_ALIGNMENT, HSA_EXECUTABLE_SYMBOL_INFO_INDIRECT_FUNCTION_OBJECT, HSA_EXECUTABLE_SYMBOL_INFO_INDIRECT_FUNCTION_CALL_CONVENTION
  ** hsa_isa_get_info: HSA_ISA_INFO_CALL_CONVENTION_COUNT, HSA_ISA_INFO_CALL_CONVENTION_INFO_WAVEFRONT_SIZE, HSA_ISA_INFO_CALL_CONVENTION_INFO_WAVEFRONTS_PER_COMPUTE_UNIT
=======
1) hsa_code_symbol_get_info: HSA_CODE_SYMBOL_INFO_VARIABLE_ALIGNMENT, HSA_CODE_SYMBOL_INFO_INDIRECT_FUNCTION_CALL_CONVENTION
2) hsa_executable_symbol_get_info: HSA_EXECUTABLE_SYMBOL_INFO_VARIABLE_ALIGNMENT, HSA_EXECUTABLE_SYMBOL_INFO_INDIRECT_FUNCTION_OBJECT, HSA_EXECUTABLE_SYMBOL_INFO_INDIRECT_FUNCTION_CALL_CONVENTION
3) hsa_isa_get_info: HSA_ISA_INFO_CALL_CONVENTION_COUNT, HSA_ISA_INFO_CALL_CONVENTION_INFO_WAVEFRONT_SIZE, HSA_ISA_INFO_CALL_CONVENTION_INFO_WAVEFRONTS_PER_COMPUTE_UNIT
>>>>>>> f2b9ed11525cafbebd079fd8deff41450855afc0

#### Known Issues

* Signals do not support multiple concurrent HOST waiters unless the the environment variable HSA_ENABLE_INTTERUPT=0.
* hsa_agent_get_exception_policies is not implemented.
* Image import/export/copy/fill only support image created with memory from host accessible region.
* Coarse grain memory usage may claim one user mode queue internally to do memory copy and reduce the number of max queue that can be created.
* hsa_memory_allocate can an return invalid status when an allocation size of 0 bytes is specified.
* hsa_system_get_extension_table is not implemented for HSA_EXTENSION_AMD_PROFILER.
* hsa_ext_image_copy only support source and destination with the same image format. It does not support SRGBA to linear RGBA conversion and vice versa.
* Acquire and release only synchronize on segment of the operation, matchng SysArch 1.0 provisonal.
* Code objects can only be loaded once.

### Disclaimer

The information contained herein is for informational purposes only, and is subject to change without notice. While every precaution has been taken in the preparation of this document, it may contain technical inaccuracies, omissions and typographical errors, and AMD is under no obligation to update or otherwise correct this information. Advanced Micro Devices, Inc. makes no representations or warranties with respect to the accuracy or completeness of the contents of this document, and assumes no liability of any kind, including the implied warranties of noninfringement, merchantability or fitness for particular purposes, with respect to the operation or use of AMD hardware, software or other products described herein. No license, including implied or arising by estoppel, to any intellectual property rights is granted by this document. Terms and limitations applicable to the purchase or use of AMD's products are as set forth in a signed agreement between the parties or in AMD's Standard Terms and Conditions of Sale.

AMD, the AMD Arrow logo, and combinations thereof are trademarks of Advanced Micro Devices, Inc. Other product names used in this publication are for identification purposes only and may be trademarks of their respective companies.

Copyright (c) 2014 Advanced Micro Devices, Inc. All rights reserved.

University of Illinois/NCSA Open Source License

Copyright (c) 2010 Apple Inc. All rights reserved.

Developed by:

LLDB Team 

http://lldb.llvm.org/ 
Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal with the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

* Redistributions of source code must retain the above copyright notice, 
  this list of conditions and the following disclaimers. 

* Redistributions in binary form must reproduce the above copyright notice, 
  this list of conditions and the following disclaimers in the 
  documentation and/or other materials provided with the distribution. 

* Neither the names of the LLDB Team, copyright holders, nor the names of  
  its contributors may be used to endorse or promote products derived from  
  this Software without specific prior written permission. 
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE SOFTWARE.

Open source files: DataBuffer.h DataBufferHeap.cpp DataBufferHeap.h DataEncoder.cpp DataEncoder.h DataExtractor.cpp DataExtractor.h DataTypes.h Dwarf.cpp Dwarf.h DWARFDebugLine.cpp DWARFDebugLine.h DWARFDefines.cpp DWARFDefines.h File.cpp File.h Flags.h lldb-dwarf.h lldb-enumerations.h lldb-types.h SmallVector.cpp SmallVector.h Stream.cpp Stream.h StreamBuffer.h StreamFile.cpp StreamFile.h SwapByteOrder.h type_traits.h

ANTLR 4 License [The BSD License] Copyright (c) 2012 Terence Parr and Sam Harwell All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met: 

Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer. 

Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution. 
Neither the name of the author nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
