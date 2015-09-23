/******************************************************************************
* University of Illinois / NCSA
* Open Source License
*
* Copyright(c) 2011 - 2015  Advanced Micro Devices, Inc.
* All rights reserved.
*
* Developed by:
* Advanced Micro Devices, Inc.
* www.amd.com
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files(the "Software"), to deal
* with the Software without restriction, including without limitation the
* rights to use, copy, modify, merge, publish, distribute, sublicense, and /
* or sell copies of the Software, and to permit persons to whom the Software
* is furnished to do so, subject to the following conditions:
*
*     Redistributions of source code must retain the above copyright notice,
* this list of conditions and the following disclaimers.
*
*     Redistributions in binary form must reproduce the above copyright notice,
* this list of conditions and the following disclaimers in the documentation
* and / or other materials provided with the distribution.
*
*     Neither the names of Advanced Micro Devices, Inc, nor the
mes of its
* contributors may be used to endorse or promote products derived from this
* Software without specific prior written permission.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
* CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS WITH
* THE SOFTWARE.
******************************************************************************/

#include "xutil.hpp"

#include <cstring>
#include <errno.h>
#include <sys/stat.h>

//===----------------------------------------------------------------------===//
// File I/O.                                                                  //
//===----------------------------------------------------------------------===//

#define XFIO_DOFF O_BINARY | O_CREAT | O_EXCL | O_RDWR

#if defined(_WIN32) || defined(_WIN64)
  #define XFIO_DOFP _S_IREAD | _S_IWRITE
#else
  #define XFIO_DOFP S_IRUSR | S_IWUSR
#endif // _WIN32 || _WIN64

int
xx_open(const char *path, const int &flags)
{
  int file_descriptor = -1;

  #if defined(_WIN32) || defined(_WIN64)
    _sopen_s(&file_descriptor, path, flags, _SH_DENYNO, XFIO_DOFP);
  #else
    file_descriptor = open(path, flags, XFIO_DOFP);
  #endif // _WIN32 || _WIN64

  return file_descriptor;
}
