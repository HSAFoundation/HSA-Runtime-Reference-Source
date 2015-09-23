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

#ifndef EXT_COMMON_XUTIL_HPP
#define EXT_COMMON_XUTIL_HPP

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
//#include <sys/stat.h>

//===----------------------------------------------------------------------===//
// File I/O.                                                                  //
//===----------------------------------------------------------------------===//

#if defined(_WIN32) || defined(_WIN64)
  #include <io.h>
  #include <share.h>

  #define xx_lseek _lseek
  #define xx_close _close
  #define xx_unlink _unlink

  #define xx_read(d, b, s) _read(d, b, static_cast<unsigned int>(s))
  #define xx_write(d, b, s) _write(d, b, static_cast<unsigned int>(s))

  #define X_TMPNAM_MAX 1024
#else
  #include <unistd.h>

  #define xx_lseek lseek
  #define xx_close close
  #define xx_unlink unlink

  #define xx_read(d, b, s) read(d, b, s)
  #define xx_write(d, b, s) write(d, b, s)

  #define X_TMPNAM_MAX 6

  #ifndef O_BINARY
    #define O_BINARY 0
  #else
    #error "O_BINARY flag must not be defined"
  #endif // O_BINARY
#endif // _WIN32 || _WIN64

int xx_open(const char *path, const int &flags);

//===----------------------------------------------------------------------===//
// Standard I/O.                                                              //
//===----------------------------------------------------------------------===//

#if defined(_WIN32) || defined(_WIN64)
  #define x_va_copy(d, s) (d) = (s)
  #define x_vscprintf(f, a) _vscprintf(f, a)
  #define x_vsnprintf(s, n, f, a) vsnprintf_s(s, n, n, f, a)
#else
  #define x_va_copy(d, s) va_copy(d, s)
  #define x_vscprintf(f, a) vsnprintf(NULL, 0, f, a)
  #define x_vsnprintf(s, n, f, a) vsnprintf(s, n, f, a)
#endif // _WIN32 || _WIN64

#endif // EXT_COMMON_XUTIL_HPP
