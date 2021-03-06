/*********************                                                        */
/*! \file ffs.c
 ** \verbatim
 ** Original author: mdeters
 ** Major contributors: none
 ** Minor contributors (to current version): none
 ** This file is part of CVC4.
 ** Copyright (c) 2009-2013  New York University and The University of Iowa
 ** See the file COPYING in the top-level source directory for licensing
 ** information.\endverbatim
 **
 ** \brief Replacement for ffs() for systems without it (like Win32)
 **
 ** Replacement for ffs() for systems without it (like Win32).
 **/

#include "cvc4_private.h"

#include "lib/ffs.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

int ffs(int i) {
  long mask = 0x1;
  int pos = 1;
  while(pos <= sizeof(int) * 8) {
    if((mask & i) != 0) {
      return pos;
    }
    ++pos;
    mask <<= 1;
  }
  return 0;
}

#ifdef __cplusplus
}/* extern "C" */
#endif /* __cplusplus */
