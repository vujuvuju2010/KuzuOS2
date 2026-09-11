/* Copyright 2001-2004 Roger Dingledine.
 * Copyright (c) 2004-2006, Roger Dingledine, Nick Mathewson.
 * Copyright (c) 2007-2021, The Tor Project, Inc. */
/* See LICENSE for licensing information */

#include "orconfig.h"
#ifdef ENABLE_RESTART_DEBUGGING
#include <stdlib.h>
#endif

/**
 * \file tor_main.c
 * \brief Stub module containing a main() function.
 *
 * We keep the main function in a separate module so that the unit
 * tests, which have their own main()s, can link against main.c.
 **/

/* In the KuzuOS port we don't use this file's main(); our ELF entry
 * point is src/tor_port/tor_main.c:_start, which calls tor_run_main().
 * To avoid pulling in an extra main() symbol and to satisfy the linker,
 * we provide a dummy tor_main() implementation here.
 */
int tor_main(int argc, char *argv[])
{
  (void)argc;
  (void)argv;
  /* Real startup is handled via tor_run_main() in tor_stubs.c. */
  return 0;
}

