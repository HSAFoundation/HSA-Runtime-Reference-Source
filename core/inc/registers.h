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

// This file is used only for open source cmake builds, if we hardcode the
// register values in amd_hw_aql_command_processor.cpp then this file won't
// be required. For now we are using this file where register details are 
// spelled out in the structs/unions below.
#ifndef HSA_RUNTME_CORE_INC_REGISTERS_H_
#define HSA_RUNTME_CORE_INC_REGISTERS_H_

typedef enum SQ_RSRC_BUF_TYPE {
SQ_RSRC_BUF                              = 0x00000000,
SQ_RSRC_BUF_RSVD_1                       = 0x00000001,
SQ_RSRC_BUF_RSVD_2                       = 0x00000002,
SQ_RSRC_BUF_RSVD_3                       = 0x00000003,
} SQ_RSRC_BUF_TYPE;

typedef enum BUF_DATA_FORMAT {
BUF_DATA_FORMAT_INVALID                  = 0x00000000,
BUF_DATA_FORMAT_8                        = 0x00000001,
BUF_DATA_FORMAT_16                       = 0x00000002,
BUF_DATA_FORMAT_8_8                      = 0x00000003,
BUF_DATA_FORMAT_32                       = 0x00000004,
BUF_DATA_FORMAT_16_16                    = 0x00000005,
BUF_DATA_FORMAT_10_11_11                 = 0x00000006,
BUF_DATA_FORMAT_11_11_10                 = 0x00000007,
BUF_DATA_FORMAT_10_10_10_2               = 0x00000008,
BUF_DATA_FORMAT_2_10_10_10               = 0x00000009,
BUF_DATA_FORMAT_8_8_8_8                  = 0x0000000a,
BUF_DATA_FORMAT_32_32                    = 0x0000000b,
BUF_DATA_FORMAT_16_16_16_16              = 0x0000000c,
BUF_DATA_FORMAT_32_32_32                 = 0x0000000d,
BUF_DATA_FORMAT_32_32_32_32              = 0x0000000e,
BUF_DATA_FORMAT_RESERVED_15              = 0x0000000f,
} BUF_DATA_FORMAT;

typedef enum BUF_NUM_FORMAT {
BUF_NUM_FORMAT_UNORM                     = 0x00000000,
BUF_NUM_FORMAT_SNORM                     = 0x00000001,
BUF_NUM_FORMAT_USCALED                   = 0x00000002,
BUF_NUM_FORMAT_SSCALED                   = 0x00000003,
BUF_NUM_FORMAT_UINT                      = 0x00000004,
BUF_NUM_FORMAT_SINT                      = 0x00000005,
BUF_NUM_FORMAT_SNORM_OGL__SI__CI         = 0x00000006,
BUF_NUM_FORMAT_RESERVED_6__VI            = 0x00000006,
BUF_NUM_FORMAT_FLOAT                     = 0x00000007,
} BUF_NUM_FORMAT;

typedef enum SQ_SEL_XYZW01 {
SQ_SEL_0                                 = 0x00000000,
SQ_SEL_1                                 = 0x00000001,
SQ_SEL_RESERVED_0                        = 0x00000002,
SQ_SEL_RESERVED_1                        = 0x00000003,
SQ_SEL_X                                 = 0x00000004,
SQ_SEL_Y                                 = 0x00000005,
SQ_SEL_Z                                 = 0x00000006,
SQ_SEL_W                                 = 0x00000007,
} SQ_SEL_XYZW01;

	union COMPUTE_TMPRING_SIZE {
	struct {
#if		defined(LITTLEENDIAN_CPU)
		unsigned int                           WAVES : 12;
		unsigned int                        WAVESIZE : 13;
		unsigned int                                 : 7;
#elif		defined(BIGENDIAN_CPU)
		unsigned int                                 : 7;
		unsigned int                        WAVESIZE : 13;
		unsigned int                           WAVES : 12;
#endif
	} bitfields, bits;
	unsigned int	u32All;
	signed int	i32All;
	float	f32All;
	};


	union SQ_BUF_RSRC_WORD0 {
	struct {
#if		defined(LITTLEENDIAN_CPU)
		unsigned int                    BASE_ADDRESS : 32;
#elif		defined(BIGENDIAN_CPU)
		unsigned int                    BASE_ADDRESS : 32;
#endif
	} bitfields, bits;
	unsigned int	u32All;
	signed int	i32All;
	float	f32All;
	};


	union SQ_BUF_RSRC_WORD1 {
	struct {
#if		defined(LITTLEENDIAN_CPU)
		unsigned int                 BASE_ADDRESS_HI : 16;
		unsigned int                          STRIDE : 14;
		unsigned int                   CACHE_SWIZZLE : 1;
		unsigned int                  SWIZZLE_ENABLE : 1;
#elif		defined(BIGENDIAN_CPU)
		unsigned int                  SWIZZLE_ENABLE : 1;
		unsigned int                   CACHE_SWIZZLE : 1;
		unsigned int                          STRIDE : 14;
		unsigned int                 BASE_ADDRESS_HI : 16;
#endif
	} bitfields, bits;
	unsigned int	u32All;
	signed int	i32All;
	float	f32All;
	};


	union SQ_BUF_RSRC_WORD2 {
	struct {
#if		defined(LITTLEENDIAN_CPU)
		unsigned int                     NUM_RECORDS : 32;
#elif		defined(BIGENDIAN_CPU)
		unsigned int                     NUM_RECORDS : 32;
#endif
	} bitfields, bits;
	unsigned int	u32All;
	signed int	i32All;
	float	f32All;
	};


	union SQ_BUF_RSRC_WORD3 {
	struct {
#if		defined(LITTLEENDIAN_CPU)
                unsigned int                       DST_SEL_X : 3;
                unsigned int                       DST_SEL_Y : 3;
                unsigned int                       DST_SEL_Z : 3;
                unsigned int                       DST_SEL_W : 3;
                unsigned int                      NUM_FORMAT : 3;
                unsigned int                     DATA_FORMAT : 4;
                unsigned int                    ELEMENT_SIZE : 2;
                unsigned int                    INDEX_STRIDE : 2;
                unsigned int                  ADD_TID_ENABLE : 1;
                unsigned int                     ATC__CI__VI : 1;
                unsigned int                     HASH_ENABLE : 1;
                unsigned int                            HEAP : 1;
                unsigned int                   MTYPE__CI__VI : 3;
                unsigned int                            TYPE : 2;
#elif		defined(BIGENDIAN_CPU)
                unsigned int                            TYPE : 2;
                unsigned int                   MTYPE__CI__VI : 3;
                unsigned int                            HEAP : 1;
                unsigned int                     HASH_ENABLE : 1;
                unsigned int                     ATC__CI__VI : 1;
                unsigned int                  ADD_TID_ENABLE : 1;
                unsigned int                    INDEX_STRIDE : 2;
                unsigned int                    ELEMENT_SIZE : 2;
                unsigned int                     DATA_FORMAT : 4;
                unsigned int                      NUM_FORMAT : 3;
                unsigned int                       DST_SEL_W : 3;
                unsigned int                       DST_SEL_Z : 3;
                unsigned int                       DST_SEL_Y : 3;
                unsigned int                       DST_SEL_X : 3;
#endif
	} bitfields, bits;
	unsigned int	u32All;
	signed int	i32All;
	float	f32All;
	};

#endif  // header guard