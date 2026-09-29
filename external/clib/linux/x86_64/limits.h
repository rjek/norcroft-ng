/* Copyright 1996 Acorn Computers Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma force_top_level
#pragma include_only_once

/* limits.h: ISO 'C' (9899:1999) library header, section 5.2.4.2.1 */
/* Copyright (C) Codemist Ltd., 1988 */
/* Copyright (C) Acorn Computers Ltd. 1991, 1992 */
/* version 3.01 */

#ifndef __limits_h
#define __limits_h

#define CHAR_BIT 8
    /* max number of bits for smallest object that is not a bit-field (byte) */
#define SCHAR_MIN (-0x80)
    /* mimimum value for an object of type signed char */
#define SCHAR_MAX 0x7F
    /* maximum value for an object of type signed char */
#define UCHAR_MAX 0xFF
    /* maximum value for an object of type unsigned char */
/* Plain char is signed, as the System V ABI requires, unless the       */
/* compiler has been told otherwise.                                    */
#ifndef __CHAR_UNSIGNED__
#define CHAR_MIN (-0x80)
    /* minimum value for an object of type char */
#define CHAR_MAX 0x7F
    /* maximum value for an object of type char */
#else
#define CHAR_MIN 0
    /* minimum value for an object of type char */
#define CHAR_MAX 0xFF
    /* maximum value for an object of type char */
#endif
#define MB_LEN_MAX 1
    /* maximum number of bytes in a multibyte character, */
    /* for any supported locale */

#define SHRT_MIN  (-0x8000)
    /* minimum value for an object of type short int */
#define SHRT_MAX  0x7FFF
    /* maximum value for an object of type short int */
#define USHRT_MAX 0xFFFF
    /* maximum value for an object of type unsigned short int */
#define INT_MIN   (~0x7FFFFFFF)
    /* minimum value for an object of type int */
#define INT_MAX   0x7FFFFFFF
    /* maximum value for an object of type int */
#define UINT_MAX  0xFFFFFFFF
    /* maximum value for an object of type unsigned int */
#define LONG_MIN  (~0x7FFFFFFFFFFFFFFFL)
    /* minimum value for an object of type long int */
#define LONG_MAX  0x7FFFFFFFFFFFFFFFL
    /* maximum value for an object of type long int */
#define ULONG_MAX 0xFFFFFFFFFFFFFFFFUL
    /* maximum value for an object of type unsigned long int */
/* (long long is supported in every mode, so these always are.)        */
#define LLONG_MIN (~0x7FFFFFFFFFFFFFFFLL)
    /* minimum value for an object of type long long int */
#define LLONG_MAX 0x7FFFFFFFFFFFFFFFLL
    /* maximum value for an object of type long long int */
#define ULLONG_MAX 0xFFFFFFFFFFFFFFFFULL
    /* maximum value for an object of type unsigned long long int */

#endif

/* end of limits.h */
