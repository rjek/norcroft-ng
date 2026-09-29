/*
 * ccx86_64/options.h -- compiler configuration options for x86-64 Linux.
 * SPDX-Licence-Identifier: Apache-2.0
 */

#ifndef _options_LOADED
#define _options_LOADED

#include "toolver.h"
#define NON_RELEASE_VSN TOOLVER_ARMCC

#define TARGET_SYSTEM     "Linux"
#define TARGET_IS_UNIX    1

#define TARGET_ENDIANNESS_CONFIGURABLE 1
#define TARGET_DEFAULT_BIGENDIAN 0

#ifdef COMPILING_ON_UNIX
#  define HOST_WANTS_NO_BANNER 1
#endif

/* There is no object file writer yet: '-c' compiles to a temporary      */
/* assembler file, which the driver then passes to the system assembler. */
#define NO_OBJECT_OUTPUT  1
#define NO_OBJECT_OUTPUT2 1

/* The system C compiler is used as the linker, so that it supplies the  */
/* C runtime startup files and libraries.  The code is position          */
/* independent, so this makes a PIE if that is the system's default.     */

#define DRIVER_ENV \
    { 0, (KEY_LINK), 0,                                                   \
      "/usr/include/x86_64-linux-gnu", "/usr/include/x86_64-linux-gnu",   \
      "", "/", "", "", "lst",                                             \
      "as --64",                                                          \
      "cc", "a.out", "",                                                 \
      "", "", "",                                                         \
      "", "", "", "", "", ""                                              \
    }

/* After the multiarch directory above.                                 */
#define DRIVER_EXTRA_INCLUDES { "/usr/include" }

#ifndef DRIVER_OPTIONS
#  define DRIVER_OPTIONS { "-D__unix", "-D__unix__", "-D__linux", \
                           "-D__linux__", NULL }
#endif

/* The usual defaults, plus: system headers rely on undefined macros    */
/* being 0 in #if.                                                      */
#define SUPPRESS_DEFAULT_LIST \
    X(ShortWarn) \
    X(StructPadding) \
    X(GuardedInclude) \
    X(PPNoSysIncludeCheck) \
    X(ImplicitCtor) \
    X(ImplicitNarrowing) \
    X(LowerInWider) \
    X(Future) \
    X(CFrontCaller) \
    X(StructAssign) \
    X(PPUndefInIf)

/* As C99 (and system headers) expect: 1e10000 is infinity.             */
#define OVERLARGE_FP_CONSTANTS_ARE_INFINITE 1

#ifndef RELEASE_VSN
#  define ENABLE_ALL          1 /* -- to enable all debugging options */
#endif

#define TARGET_STACK_MOVES_ONCE
#define target_stack_moves_once 1

#ifndef MSG_TOOL_NAME
#  define MSG_TOOL_NAME  "ncc"
#endif

#endif

/* end of ccx86_64/options.h */
