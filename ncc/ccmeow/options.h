/*
 * C compiler file ccmeow/options.h
 * Copyright (C) Codemist Ltd., 1988-94.
 * SPDX-Licence-Identifier: Apache-2.0
 *
 * Options for the MEOW C compiler, meowcc.
 */

#ifndef _options_LOADED
#define _options_LOADED

#include "toolver.h"
#define NON_RELEASE_VSN TOOLVER_TCC

#define DISABLE_ERRORS

#define PCS_DEFAULTS (PCS_NOSTACKCHECK|PCS_NOFP)

#define TARGET_SYSTEM     ""

#ifdef COMPILING_ON_UNIX
#define DRIVER_ENV     { \
      0, (KEY_LINK), 0, \
      "/usr/local/lib/meow", "/usr/local/lib/meow", "", "/", "/usr/local/lib/meow", "", "lst", \
      "mas", \
      "mld", NULL, "", \
      "", "", "", \
      "libmeow.o", "hostlib.o", "libmeow.o", "", "", "" \
}
#else
#error Unknown host
#endif

#define C_INC_VAR  "MEOWINC"
#define C_LIB_VAR  "MEOWLIB"

#ifndef DRIVER_OPTIONS
#  define DRIVER_OPTIONS     {"-D__meow", NULL}
#endif

#ifndef RELEASE_VSN
#  define ENABLE_ALL          1 /* -- to enable all debugging options */
#endif

#define HOST_WANTS_NO_BANNER 1

#define MSG_TOOL_NAME  "armcc"  /* used to load correct NLS message file */

#endif

/* end of ccmeow/options.h */
