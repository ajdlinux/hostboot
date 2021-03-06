/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/import/chips/p9/common/include/p9_const_common.H $        */
/*                                                                        */
/* OpenPOWER HostBoot Project                                             */
/*                                                                        */
/* Contributors Listed Below - COPYRIGHT 2015,2017                        */
/* [+] International Business Machines Corp.                              */
/*                                                                        */
/*                                                                        */
/* Licensed under the Apache License, Version 2.0 (the "License");        */
/* you may not use this file except in compliance with the License.       */
/* You may obtain a copy of the License at                                */
/*                                                                        */
/*     http://www.apache.org/licenses/LICENSE-2.0                         */
/*                                                                        */
/* Unless required by applicable law or agreed to in writing, software    */
/* distributed under the License is distributed on an "AS IS" BASIS,      */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or        */
/* implied. See the License for the specific language governing           */
/* permissions and limitations under the License.                         */
/*                                                                        */
/* IBM_PROLOG_END_TAG                                                     */


/*! \brief These are macros used for the scom_addresses.H
 *
 *  Provides macro defintions for defining scom constants
 *  for assembly and C
 *
 */

// - HWP metadata

///
/// @file const_common.H
/// @brief These are macros used for the scom_addresses.H
///
// *HWP HWP Owner: Ben Gass <bgass@us.ibm.com>
// *HWP FW Owner: ? <?>
// *HWP Team: SoA
// *HWP Level: 1
// *HWP Consumed by: XX:XX


#ifndef __CONST_COMMON_H
#define __CONST_COMMON_H

#if defined __ASSEMBLER__

//This probably won't work :(
//  Not sure you can do .set for the same name.
//  If so, it probably takes the last one, which would be the incorrect one
//#define FIXREG8(name, expr, unit, meth, newexpr) .set name, (newexpr)
//#define FIXREG32(name, expr, unit, meth, newexpr) .set name, (newexpr)
//#define FIXREG64(name, expr, unit, meth, newexpr) .set name, (newexpr)
//
//#define CONST_UINT8_T(name, expr, unit, meth) .set name, (expr)
//#define CONST_UINT32_T(name, expr, unit, meth) .set name, (expr)
//#define CONST_UINT64_T(name, expr, unit, meth) .set name, (expr)

#define RULL(x) x

#elif defined __cplusplus

#include <stdint.h>

template <typename T, uint64_t UNIT, uint64_t METH, T REG >
struct has_fixup
{
    static const T value = T(0);
};

template <typename T, uint64_t UNIT, uint64_t METH, T REG, uint64_t FLD >
struct has_fixfld
{
    static const uint8_t value = 255;
};

const uint32_t N0_CHIPLET_ID = 2;
const uint32_t N1_CHIPLET_ID = 3;
const uint32_t N2_CHIPLET_ID = 4;
const uint32_t N3_CHIPLET_ID = 5;
const uint32_t XB_CHIPLET_ID = 6;
const uint32_t MC01_CHIPLET_ID = 7;
const uint32_t MC23_CHIPLET_ID = 8;
const uint32_t OB0_CHIPLET_ID = 9;
const uint32_t OB1_CHIPLET_ID = 10;
const uint32_t OB2_CHIPLET_ID = 11;
const uint32_t OB3_CHIPLET_ID = 12;
const uint32_t PCI0_CHIPLET_ID = 13;
const uint32_t PCI1_CHIPLET_ID = 14;
const uint32_t PCI2_CHIPLET_ID = 15;
const uint32_t EC0_CHIPLET_ID = 0x20;
const uint32_t EC23_CHIPLET_ID = 0x37;
const uint32_t EQ0_CHIPLET_ID = 0x10;
const uint32_t EQ5_CHIPLET_ID = 0x15;
#define FIXREG8(name, expr, unit, meth, newexpr) template<> struct has_fixup<uint8_t,unit,meth,expr> { static const uint8_t value = newexpr; };
#define FIXREG32(name, expr, unit, meth, newexpr) template<> struct has_fixup<uint32_t,unit,meth,expr> { static const uint32_t value = newexpr; };
#define FIXREG64(name, expr, unit, meth, newexpr) template<> struct has_fixup<uint64_t,unit,meth,expr> { static const uint64_t value = newexpr; };

#define REG8(name, expr, unit, meth) static const uint8_t name = has_fixup<uint8_t,unit,meth,expr>::value ? has_fixup<uint8_t,unit,meth,expr>::value : expr;
#define REG32(name, expr, unit, meth) static const uint32_t name = has_fixup<uint32_t,unit,meth,expr>::value ? has_fixup<uint32_t,unit,meth,expr>::value : expr;
#define REG64(name, expr, unit, meth) static const uint64_t name = has_fixup<uint64_t,unit,meth,expr>::value ? has_fixup<uint64_t,unit,meth,expr>::value : expr;

#define FIXREG8_FLD(name, expr, unit, meth, fld, newexpr) template<> struct has_fixfld<uint8_t,unit,meth,expr,fld> { static const uint8_t value = newexpr; };
#define FIXREG32_FLD(name, expr, unit, meth, fld, newexpr) template<> struct has_fixfld<uint32_t,unit,meth,expr,fld> { static const uint32_t value = newexpr; };
#define FIXREG64_FLD(name, expr, unit, meth, fld, newexpr) template<> struct has_fixfld<uint64_t,unit,meth,expr,fld> { static const uint64_t value = newexpr; };

#define REG8_FLD(name, expr, unit, meth, fld) static const uint8_t name = has_fixfld<uint8_t,unit,meth,expr,fld>::value != 255 ? has_fixfld<uint8_t,unit,meth,expr,fld>::value : expr;
#define REG32_FLD(name, expr, unit, meth, fld) static const uint8_t name = has_fixfld<uint32_t,unit,meth,expr,fld>::value != 255 ? has_fixfld<uint32_t,unit,meth,expr,fld>::value : expr;
#define REG64_FLD(name, expr, unit, meth, fld) static const uint8_t name = has_fixfld<uint64_t,unit,meth,expr,fld>::value != 255 ? has_fixfld<uint64_t,unit,meth,expr,fld>::value : expr;

#define RULL(x) x##ull

//Remove the chiplet id from any register so that it can
//be used with pu.perv target type
#define FORCE_PERV(val) (RULL(0xFFFFFFFF00FFFFFF) & val)

#else // C code

// CONST_UINT[8,3,64]_T() can't be used in C code/headers; Use
//
//     #define <symbol> <value> [ or ULL(<value>) for 64-bit constants

#define RULL(x) x##ull

#endif  // __ASSEMBLER__

#endif  // __CONST_COMMON_H

