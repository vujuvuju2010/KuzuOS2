/* Copyright (c) 2003-2004, Roger Dingledine
 * Copyright (c) 2004-2006, Roger Dingledine, Nick Mathewson.
 * Copyright (c) 2007-2021, The Tor Project, Inc. */
/* See LICENSE for licensing information */

/**
 * \file compat_mutex.c
 *
 * \brief Portable wrapper for platform mutex implementations.
 **/

#include "lib/lock/compat_mutex.h"
#include "lib/malloc/malloc.h"

void tor_locking_init(void) {
}

void tor_mutex_init(tor_mutex_t *m) {
  if (!m) return;
  m->_unused = 0;
  m->lock_count = 0;
}

void tor_mutex_init_nonrecursive(tor_mutex_t *m) {
  tor_mutex_init(m);
}

void tor_mutex_acquire(tor_mutex_t *m) {
  if (!m) return;
  m->lock_count++;
}

void tor_mutex_release(tor_mutex_t *m) {
  if (!m) return;
  m->lock_count--;
}

void tor_mutex_uninit(tor_mutex_t *m) {
  (void)m;
}

tor_mutex_t *tor_mutex_new(void) {
  tor_mutex_t *m = tor_malloc_zero(sizeof(tor_mutex_t));
  tor_mutex_init(m);
  return m;
}

tor_mutex_t *tor_mutex_new_nonrecursive(void) {
  tor_mutex_t *m = tor_malloc_zero(sizeof(tor_mutex_t));
  tor_mutex_init_nonrecursive(m);
  return m;
}

void tor_mutex_free_(tor_mutex_t *m) {
  if (!m) return;
  tor_mutex_uninit(m);
  tor_free(m);
}
