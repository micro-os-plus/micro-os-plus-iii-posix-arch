/*
 * This file is part of the µOS++ distribution.
 *   (https://github.com/micro-os-plus)
 * Copyright (c) 2016 Liviu Ionescu.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * This file is part of the CMSIS++ proposal, intended as a CMSIS
 * replacement for C++ applications.
 *
 * If contains the scheduler implementation that uses
 * functions from the POSIX API (macOS and GNU/Linux).
 *
 * This file is included in all src/os-*.cpp files.
 */

#ifndef CMSIS_PLUS_RTOS_PORT_OS_INLINES_H_
#define CMSIS_PLUS_RTOS_PORT_OS_INLINES_H_

// ----------------------------------------------------------------------------

#include <cmsis-plus/os-app-config.h>
#include <cmsis-plus/rtos/os-c-decls.h>

// ----------------------------------------------------------------------------

#ifdef  __cplusplus

// ----------------------------------------------------------------------------

#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>
#include <sys/time.h>

// For Linux
#include <unistd.h>

#include <cmsis-plus/diag/trace.h>

// ----------------------------------------------------------------------------

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

#if defined(__clang__)
#pragma clang diagnostic ignored "-Wc++98-compat"
#endif

// ----------------------------------------------------------------------------

extern "C" uint32_t signal_nesting;

namespace os
{
  namespace rtos
  {
    namespace port
    {
      // ----------------------------------------------------------------------

      namespace scheduler
      {

        inline void
        __attribute__((always_inline))
        greeting (void)
        {
          /* struct */ utsname name;
          if (::uname (&name) != -1)
            {
              trace::printf ("POSIX synthetic, running on %s %s %s",
                             name.machine, name.sysname, name.release);
            }
          else
            {
              trace::printf ("POSIX synthetic");
            }

          trace::puts ("; non-preemptive.");
        }

        inline port::scheduler::state_t
        __attribute__((always_inline))
        lock (void)
        {
          return locked (state::locked);
        }

        inline port::scheduler::state_t
        __attribute__((always_inline))
        unlock (void)
        {
          return locked (state::unlocked);
        }

        inline bool
        __attribute__((always_inline))
        locked (void)
        {
          return lock_state != state::unlocked;
        }

        inline void
        __attribute__((always_inline))
        wait_for_interrupt (void)
        {
#if defined(OS_TRACE_RTOS_THREAD_CONTEXT)
          trace::printf ("%s() \n", __func__);
#endif
          pause ();
        }

      } /* namespace scheduler */

      namespace interrupts
      {

        inline bool
        __attribute__((always_inline))
        in_handler_mode (void)
        {
          return (signal_nesting > 0);
        }

        inline bool
        __attribute__((always_inline))
        is_priority_valid (void)
        {
          return true;
        }

        // Enter an IRQ critical section
        inline rtos::interrupts::state_t
        __attribute__((always_inline))
        critical_section::enter (void)
        {
          sigset_t old;
          sigprocmask (SIG_BLOCK, &clock_set, &old);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
          return sigismember(&old, clock::signal_number);
#pragma GCC diagnostic pop
        }

        // Exit an IRQ critical section
        inline void
        __attribute__((always_inline))
        critical_section::exit (rtos::interrupts::state_t state)
        {
          sigprocmask (state ? SIG_BLOCK : SIG_UNBLOCK, &clock_set, nullptr);
        }

        // ====================================================================

        // Enter an IRQ uncritical section
        inline rtos::interrupts::state_t
        __attribute__((always_inline))
        uncritical_section::enter (void)
        {
          sigset_t old;
          sigprocmask (SIG_UNBLOCK, &clock_set, &old);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
          return sigismember(&old, clock::signal_number);
#pragma GCC diagnostic pop
        }

        // Exit an IRQ critical section
        inline void
        __attribute__((always_inline))
        uncritical_section::exit (rtos::interrupts::state_t state)
        {
          sigprocmask (state ? SIG_BLOCK : SIG_UNBLOCK, &clock_set, nullptr);
        }

      } /* namespace interrupts */

      // ======================================================================

      namespace this_thread
      {
        inline void
        __attribute__((always_inline))
        prepare_suspend (void)
        {
        }

      } /* namespace this_thread */

    // ========================================================================

    } /* namespace port */
  } /* namespace rtos */
} /* namespace os */

#pragma GCC diagnostic pop

// ----------------------------------------------------------------------------

#endif /* __cplusplus */

// ----------------------------------------------------------------------------

#endif /* CMSIS_PLUS_RTOS_PORT_OS_INLINES_H_ */
