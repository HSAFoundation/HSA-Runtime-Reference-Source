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

#ifndef HSA_RUNTME_CORE_UTIL_FUNCTION_TRAITS_H_
#define HSA_RUNTME_CORE_UTIL_FUNCTION_TRAITS_H_

#include <type_traits>

//Hold a pair of types
template<typename T1, typename T2> struct tpair { typedef T1 A; typedef T2 B; };

//List of types defaulting to void
template<typename T0=void, typename T1=void, typename T2=void, typename T3=void, typename T4=void, typename T5=void, typename T6=void, typename T7=void, typename T8=void, typename T9=void, typename T10=void, typename T11=void, typename T12=void, typename T13=void, typename T14=void, typename T15=void, typename T16=void, typename T17=void, typename T18=void, typename T19=void, typename T20=void>
struct tlist
{
	typedef tpair<T0, tpair<T1, tpair<T2, tpair<T3, tpair<T4, tpair<T5, tpair<T6, tpair<T7, tpair<T8, tpair<T9, tpair<T10, tpair<T11, tpair<T12, tpair<T13, tpair<T14, tpair<T15, tpair<T16, tpair<T17, tpair<T18, tpair<T19, tpair<T20, void>>>>>>>>>>>>>>>>>>>>> list;
};

//Indexes a tlist list of types returning the type at index
template<typename list, int index> struct tindex { typedef typename tindex<typename list::B, index-1>::type type; };
template<typename list> struct tindex<list, 0> { typedef typename list::A type; };

//Counts the non-void types in a tlist
template<typename list> struct tcount { enum { value = tcount<typename list::B>::value + (std::is_same<typename list::A, void>::value ? 0 : 1) }; };
template<> struct tcount<void> { enum { value = 0 }; };

//Converts a function pointer into a tlist
template<class T> class tfunc
{
private:
	static T *in;
public:
	template<class Ret> static tlist<Ret> getTypes(Ret());
	template<class Ret, class A1> static tlist<Ret, A1> getTypes(Ret(A1));
	template<class Ret, class A1, class A2> static tlist<Ret, A1, A2> getTypes(Ret(A1, A2));
	template<class Ret, class A1, class A2, class A3> static tlist<Ret, A1, A2, A3> getTypes(Ret(A1, A2, A3));
	template<class Ret, class A1, class A2, class A3, class A4> static tlist<Ret, A1, A2, A3, A4> getTypes(Ret(A1, A2, A3, A4));
	template<class Ret, class A1, class A2, class A3, class A4, class A5> static tlist<Ret, A1, A2, A3, A4, A5> getTypes(Ret(A1, A2, A3, A4, A5));
	template<class Ret, class A1, class A2, class A3, class A4, class A5, class A6> static tlist<Ret, A1, A2, A3, A4, A5, A6> getTypes(Ret(A1, A2, A3, A4, A5, A6));
	template<class Ret, class A1, class A2, class A3, class A4, class A5, class A6, class A7> static tlist<Ret, A1, A2, A3, A4, A5, A6, A7> getTypes(Ret(A1, A2, A3, A4, A5, A6, A7));
	template<class Ret, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8> static tlist<Ret, A1, A2, A3, A4, A5, A6, A7, A8> getTypes(Ret(A1, A2, A3, A4, A5, A6, A7, A8));
	template<class Ret, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9> static tlist<Ret, A1, A2, A3, A4, A5, A6, A7, A8, A9> getTypes(Ret(A1, A2, A3, A4, A5, A6, A7, A8, A9));
	template<class Ret, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10> static tlist<Ret, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10> getTypes(Ret(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10));
	template<class Ret, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11> static tlist<Ret, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11> getTypes(Ret(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11));
	template<class Ret, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12> static tlist<Ret, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12> getTypes(Ret(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12));
	template<class Ret, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class A13> static tlist<Ret, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13> getTypes(Ret(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13));
	template<class Ret, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class A13, class A14> static tlist<Ret, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14> getTypes(Ret(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14));
	template<class Ret, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class A13, class A14, class A15> static tlist<Ret, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15> getTypes(Ret(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15));
	template<class Ret, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class A13, class A14, class A15, class A16> static tlist<Ret, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16> getTypes(Ret(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16));
	template<class Ret, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class A13, class A14, class A15, class A16, class A17> static tlist<Ret, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17> getTypes(Ret(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17));
	template<class Ret, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class A13, class A14, class A15, class A16, class A17, class A18> static tlist<Ret, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17, A18> getTypes(Ret(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17, A18));
	template<class Ret, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class A13, class A14, class A15, class A16, class A17, class A18, class A19> static tlist<Ret, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17, A18, A19> getTypes(Ret(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17, A18, A19));
	template<class Ret, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10, class A11, class A12, class A13, class A14, class A15, class A16, class A17, class A18, class A19, class A20> static tlist<Ret, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17, A18, A19, A20> getTypes(Ret(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17, A18, A19, A20));
	typedef decltype(getTypes(*in)) tlist;
};

//Provides the return type or argument type of a function indexed as a tlist
template<typename T, int index> struct arg_type { typedef typename tindex<typename tfunc<T>::tlist::list, index>::type type; };

//Returns the number of arguments to a function
template<typename T> struct arg_count { enum { value = tcount<typename tfunc<T>::tlist::list::B>::value }; };

#endif
