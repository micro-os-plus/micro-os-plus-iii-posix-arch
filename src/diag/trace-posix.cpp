/*
 * This file is part of the µOS++ distribution.
 *   (https://github.com/micro-os-plus)
 * Copyright (c) 2015-2023 Liviu Ionescu. All rights reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software
 * for any purpose is hereby granted, under the terms of the MIT license.
 *
 * If a copy of the license was not distributed with this file, it can
 * be obtained from https://opensource.org/licenses/mit/.
 */

#if defined(__APPLE__) || defined(__linux__)

// ----------------------------------------------------------------------------

#if defined(TRACE)

#if defined(OS_USE_OS_APP_CONFIG_H)
#include <cmsis-plus/os-app-config.h>
#endif

#include <cmsis-plus/diag/trace.h>
#include <cstddef>
#include <unistd.h>
#include <stdio.h>

// ----------------------------------------------------------------------------

namespace os
{
  namespace trace
  {
    // ------------------------------------------------------------------------

    void
    initialize (void)
    {
      // For POSIX no inits are required.
    }

    // ------------------------------------------------------------------------

    ssize_t
    write (const void* buf, std::size_t nbyte)
    {
#if defined(OS_USE_TRACE_POSIX_STDOUT)
      ssize_t ret = ::write (1, buf, nbyte); // Forward to STDOUT.
      // sync(); // Fails to change contexts!
      return ret;
#elif defined(OS_USE_TRACE_POSIX_STDERR)
      ssize_t ret = ::write (2, buf, nbyte); // Forward to STDERR.
      // sync(); // Fails to change contexts!
      return ret;
#elif defined(OS_USE_TRACE_POSIX_FWRITE_STDOUT)
      // Forward to buffered file STDOUT.
      // WARNING: not thread safe, sometimes it hangs.
      // TODO: add synchronisation.
      ssize_t ret = static_cast<ssize_t>(::fwrite (buf, 1, nbyte, stdout));
      return ret;
#elif defined(OS_USE_TRACE_POSIX_FWRITE_STDERR)
      // Forward to buffered file STDERR.
      // WARNING: not thread safe, sometimes it hangs.
      // TODO: add synchronisation.
      ssize_t ret = static_cast<ssize_t>(::fwrite (buf, 1, nbyte, stderr));
      return ret;
#else
#warning "No trace output channel."
      buf = buf;
      return nbyte;
#endif
    }

  } /* namespace trace */
} /* namespace os */

#endif /* defined(TRACE) */

// ----------------------------------------------------------------------------

#endif /* defined(__APPLE__) || defined(__linux__) */
