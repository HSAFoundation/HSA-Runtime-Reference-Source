HSA-Runtime-Reference-Source - MatrixMultiplication Sample
============================

This directory contains a sample for use with the open source version of the HSA runtime reference source.

##### Files & Source

* MatrixMultiplication/Makefile - Makefile
* MatrixMultiplication/main.c - Sample host code.
* MatrixMultiplication/MatMul.cl - OpenCL C kernel code
* MatrixMultiplication/MatMul.cl.o - ELF container for the AMD kernel code object created using the MatMul.cl OpenCL C kernel code.
* MatrixMultiplication/compile_and_link.sh - Shell script used to compile and link the OpenCL C kernel code into the ELF container.

### Build environment

The sample requires that the runtime is built, as specified in the core/CMakeLists.txt file. Build the sample by typing 'make' in the sample/MatrixMultiplication directory.

If a build directory other than debug64 is used to build the runtime, set the HSA_RUNTIME_LIB_PATH to that directory (default is set to ../../core/debug64).

### Building the MatMul.cl.o kernel container

* Go to the http://dri.freedesktop.org/wiki/GalliumCompute/#index3h3 website.
* Follow the directions for "Getting the source code" in the "Current Development version" subsection. Mesa is not required.
* Follow the directions for "Building".
* Execute the command 'compile_and_link.sh MatMul.cl'

### Sample execution

To execute the MatMul sample, make sure that the LD_LIBRARY_PATH environment variable contain the location of the libhsa-runtime64.so library. A successful execution will give the following output:

    Using agent: Spectre
    Loaded ELF successfully.
    ----------------------
    amd_version_major: 0
    amd_version_minor: 1
    struct_byte_size:  256
    target_chip:       12
    kernel_code_entry: 256
    ----------------------

    Matrix multiplication. C = A * B
    Matrix input A = 
     8 3 7 7
     9 1 3 7
     2 5 4 6
     3 5 9 9

    Matrix input B = 
     7 8 0 1
     3 4 6 0
     5 6 9 2
     2 0 9 3

    Matrix output C = 
     114 118 144 43
     95 94 96 36
     61 60 120 28
     99 98 192 48

    PASSED.

### Disclaimer

The information contained herein is for informational purposes only, and is subject to change without notice. While every precaution has been taken in the preparation of this document, it may contain technical inaccuracies, omissions and typographical errors, and AMD is under no obligation to update or otherwise correct this information. Advanced Micro Devices, Inc. makes no representations or warranties with respect to the accuracy or completeness of the contents of this document, and assumes no liability of any kind, including the implied warranties of noninfringement, merchantability or fitness for particular purposes, with respect to the operation or use of AMD hardware, software or other products described herein. No license, including implied or arising by estoppel, to any intellectual property rights is granted by this document. Terms and limitations applicable to the purchase or use of AMD's products are as set forth in a signed agreement between the parties or in AMD's Standard Terms and Conditions of Sale.

AMD, the AMD Arrow logo, and combinations thereof are trademarks of Advanced Micro Devices, Inc. Other product names used in this publication are for identification purposes only and may be trademarks of their respective companies.
