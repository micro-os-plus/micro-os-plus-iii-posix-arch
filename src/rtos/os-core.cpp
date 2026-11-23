/*
 * This file is part of the µOS++ project (https://micro-os-plus.github.io/).
 * Copyright (c) 2016-2025 Liviu Ionescu. All rights reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software
 * for any purpose is hereby granted, under the terms of the MIT license.
 *
 * If a copy of the license was not distributed with this file, it can
 * be obtained from https://opensource.org/licenses/mit/.
 */

#if defined(__APPLE__) || defined(__linux__)

// ----------------------------------------------------------------------------

#include <cassert>

#include <cmsis-plus/rtos/os.h>

#include <sys/time.h>
#include <time.h>

// ----------------------------------------------------------------------------

#if defined(__clang__)
#pragma clang diagnostic ignored "-Wc++98-compat"
#endif

// ----------------------------------------------------------------------------

uint32_t signal_nesting;

namespace os
{
  namespace rtos
  {
    namespace port
    {
      // ----------------------------------------------------------------------

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

      void
      context::create (void* context, void* func, void* args)
      {
        /* class */ rtos::thread::context* th_ctx
            = static_cast</* class */ rtos::thread::context*> (context);
        memset (&th_ctx->port_, 0, sizeof (th_ctx->port_));

#pragma GCC diagnostic push
#if defined(__clang__)
#elif defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wuseless-cast"
#endif
        os_impl_ucontext_t* ctx = reinterpret_cast<os_impl_ucontext_t*> (
            &(th_ctx->port_.ucontext));
#pragma GCC diagnostic pop

#if defined(OS_TRACE_RTOS_THREAD_CONTEXT)
        trace::printf ("port::context::%s() getcontext %p\n", __func__, ctx);
#endif

        if (os_impl_getcontext (ctx) != 0)
          {
            trace::printf ("port::context::%s() getcontext failed with %s\n",
                           __func__, strerror (errno));
            abort ();
          }

        // The context in itself is not needed, but makecontext()
        // requires a context obtained by getcontext().

        // getcontext() also saves the signal mask, which setcontext() and
        // swapcontext() restore, so a thread created while the clock
        // signal is blocked (in an interrupts critical section) would run
        // with it blocked forever. Start every thread with the clock
        // signal enabled, as Cortex-M threads start with interrupts
        // enabled; the other signals keep the creator's mask.
        sigdelset (&ctx->os_impl_uc_sigmask,
                   static_cast<int> (clock::signal_number));

        // Remove the parent link.
        // TODO: maybe use this to link to exit code.
        ctx->uc_link = nullptr;

        // Configure the new stack to default values.
        ctx->uc_stack.ss_sp = th_ctx->stack ().bottom ();
        ctx->uc_stack.ss_size = th_ctx->stack ().size ();
        ctx->uc_stack.ss_flags = 0;

#if defined(OS_TRACE_RTOS_THREAD_CONTEXT)
        trace::printf ("port::context::%s() makecontext %p\n", __func__, ctx);
#endif

        // POSIX defines the extra arguments as int; libucontext (on all
        // its architectures) and glibc read them as register-width
        // values, so the thread pointer can be passed directly.
#pragma GCC diagnostic push
#if defined(__clang__)
#pragma clang diagnostic ignored "-Wc++98-compat-pedantic"
#endif
        os_impl_makecontext (ctx, reinterpret_cast<func_t> (func), 1, args);
#pragma GCC diagnostic pop

        // context->port_.saved = false;
      }

#pragma GCC diagnostic pop

      // ----------------------------------------------------------------------

      namespace interrupts
      {
        // --------------------------------------------------------------------

        sigset_t clock_set;

        // --------------------------------------------------------------------

        // Enter an IRQ critical section
        rtos::interrupts::state_t
        critical_section::enter (void)
        {
#if defined(OS_TRACE_RTOS_SCHEDULER)
          trace::printf ("{c ");
#endif
          sigset_t old;
          sigprocmask (SIG_BLOCK, &clock_set, &old);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
          return sigismember (&old, clock::signal_number);
#pragma GCC diagnostic pop
        }

        // Exit an IRQ critical section
        void
        critical_section::exit (rtos::interrupts::state_t state)
        {
#if defined(OS_TRACE_RTOS_SCHEDULER)
          trace::printf (" c}");
#endif
          sigprocmask (state ? SIG_BLOCK : SIG_UNBLOCK, &clock_set, nullptr);
        }

        // ====================================================================

        // Enter an IRQ uncritical section
        rtos::interrupts::state_t
        uncritical_section::enter (void)
        {
#if defined(OS_TRACE_RTOS_SCHEDULER)
          trace::printf ("{u ");
#endif
          sigset_t old;
          sigprocmask (SIG_UNBLOCK, &clock_set, &old);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
          return sigismember (&old, clock::signal_number);
#pragma GCC diagnostic pop
        }

        // Exit an IRQ critical section
        void
        uncritical_section::exit (rtos::interrupts::state_t state)
        {
#if defined(OS_TRACE_RTOS_SCHEDULER)
          trace::printf (" u}");
#endif
          sigprocmask (state ? SIG_BLOCK : SIG_UNBLOCK, &clock_set, nullptr);
        }

      } /* namespace interrupts */

      // ----------------------------------------------------------------------

      namespace scheduler
      {

        // --------------------------------------------------------------------

        state_t lock_state;

        // --------------------------------------------------------------------

        void
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

          trace::puts ("; non-preemptive");
        }

        result_t
        initialize (void)
        {
          signal_nesting = 0;

          // Must be done before the first critical section.
          sigemptyset (&interrupts::clock_set);

#pragma GCC diagnostic push
#if defined(__clang__)
#elif defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wsign-conversion"
#endif
          sigaddset (&interrupts::clock_set, clock::signal_number);
#pragma GCC diagnostic pop

          return result::ok;
        }

        // --------------------------------------------------------------------

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

        void
        start (void)
        {
          {
            rtos::interrupts::critical_section ics;

            // Determine the next thread (unlink_head() also marks it as
            // running).
            rtos::scheduler::current_thread_
                = rtos::scheduler::ready_threads_list_.unlink_head ();

            // As the context switch does for the next threads, and as
            // the Cortex-M port does; the statistics and the switch
            // timestamp are reset by the core before calling the port.
            instrumentation::thread::active (rtos::scheduler::current_thread_);
          }

#pragma GCC diagnostic push
#if defined(__clang__)
#elif defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wuseless-cast"
#endif
          os_impl_ucontext_t* new_context
              = reinterpret_cast<os_impl_ucontext_t*> (&(
                  rtos::scheduler::current_thread_->context_.port_.ucontext));
#pragma GCC diagnostic pop

#if defined(OS_TRACE_RTOS_THREAD_CONTEXT)
          trace::printf ("port::scheduler::%s() ctx %p %s\n", __func__,
                         new_context,
                         rtos::scheduler::current_thread_->name ());
#endif

          lock_state = state::init;

#if defined NDEBUG
          os_impl_setcontext (new_context);
#else
          int res = os_impl_setcontext (new_context);
          assert (res == 0);
#endif
          abort ();
        }

        // --------------------------------------------------------------------

        state_t
        locked (state_t state)
        {
          os_assert_throw (!interrupts::in_handler_mode (), EPERM);

          state_t tmp;

          {
            rtos::interrupts::critical_section ics;

            tmp = lock_state;
            lock_state = state;
          }

          return tmp;
        }

        // --------------------------------------------------------------------

        void
        reschedule (void)
        {
          // Before the scheduler is started there is nothing to switch
          // to; threads created by static constructors only become
          // ready, and run after scheduler::start(). The core sets the
          // started flag before calling port::scheduler::start().
          if (!rtos::scheduler::started ())
            {
              return;
            }

          if (rtos::scheduler::locked ()
              && !rtos::interrupts::in_handler_mode ())
            {
              // Remember the request; the core performs it when the
              // scheduler is unlocked.
              os::rtos::scheduler::is_reschedule_pending = true;
              return;
            }

          if (rtos::scheduler::locked ()
              || rtos::interrupts::in_handler_mode ())
            {
#if defined(OS_TRACE_RTOS_THREAD_CONTEXT)
              // trace::printf ("port::scheduler::%s() deny\n", __func__);
#endif
              return;
            }

#if defined(OS_TRACE_RTOS_THREAD_CONTEXT)
          trace::printf ("port::scheduler::%s()\n", __func__);
#endif

          // For some complicated reasons, the context save/restore
          // functions must be called in the same the function.
          // The idea to inline functions does not work, since
          // the compiler does not inline functions with context calls.

          bool save = false;
          rtos::thread* old_thread;
          os_impl_ucontext_t* old_ctx;
          os_impl_ucontext_t* new_ctx;

          {
            rtos::interrupts::critical_section ics;

            // Any deferred request is performed now.
            os::rtos::scheduler::is_reschedule_pending = false;

            old_thread = rtos::scheduler::current_thread_;
            if ((old_thread->state_ == rtos::thread::state::running)
                || (old_thread->state_ == rtos::thread::state::suspended)
                || (old_thread->state_ == rtos::thread::state::ready))
              {
                save = true;
              }
#if defined(OS_TRACE_RTOS_THREAD_CONTEXT)
            trace::printf ("port::scheduler::%s() from %s %d %d\n", __func__,
                           old_thread->name (), old_thread->state_, save);
#endif

#pragma GCC diagnostic push
#if defined(__clang__)
#elif defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wuseless-cast"
#endif
            old_ctx = reinterpret_cast<os_impl_ucontext_t*> (
                &old_thread->context_.port_.ucontext);
#pragma GCC diagnostic pop

            rtos::scheduler::internal_switch_threads ();

#pragma GCC diagnostic push
#if defined(__clang__)
#elif defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wuseless-cast"
#endif
            new_ctx = reinterpret_cast<os_impl_ucontext_t*> (
                &rtos::scheduler::current_thread_->context_.port_.ucontext);
#pragma GCC diagnostic pop
          }

          if (old_ctx != new_ctx)
            {
              if (save)
                {
#if defined(OS_TRACE_RTOS_THREAD_CONTEXT)
                  trace::printf (
                      "port::scheduler::%s() swapcontext %s -> %s \n",
                      __func__, old_thread->name (),
                      rtos::scheduler::current_thread_->name ());
#endif
                  if (os_impl_swapcontext (old_ctx, new_ctx) != 0)
                    {
                      trace::printf (
                          "port::scheduler::%s() swapcontext failed with %s\n",
                          __func__, strerror (errno));
                      abort ();
                    }
                }
              else
                {
#if defined(OS_TRACE_RTOS_THREAD_CONTEXT)
                  trace::printf ("port::scheduler::%s() setcontext %s\n",
                                 __func__,
                                 rtos::scheduler::current_thread_->name ());
#endif
                  // context->port_.saved = false;
                  if (os_impl_setcontext (new_ctx) != 0)
                    {
                      trace::printf (
                          "port::scheduler::%s() setcontext failed with %s\n",
                          __func__, strerror (errno));
                      abort ();
                    }
                }
            }
          else
            {
#if defined(OS_TRACE_RTOS_THREAD_CONTEXT)
              trace::printf ("port::scheduler::%s() same %s\n", __func__,
                             old_thread->name ());
#endif
            }
        }

#pragma GCC diagnostic pop

        // --------------------------------------------------------------------

      } /* namespace scheduler */

      // ----------------------------------------------------------------------

      static void
      systick_clock_signal_handler (int signum)
      {
#if defined(OS_TRACE_RTOS_SYSCLOCK_TICK)
        trace::printf ("{i ");
#endif

        // Installed only for the clock signal.
        assert (signum == static_cast<int> (clock::signal_number));
        (void)signum;

        signal_nesting++;
        // Call the ticks timer ISR.
        os_systick_handler ();
        signal_nesting--;
#if defined(OS_TRACE_RTOS_SYSCLOCK_TICK)
        trace::printf (" i}");
#endif
      }

      // ======================================================================

      void
      clock_systick::start (void)
      {
        // set handler
        struct sigaction sa;
#if defined(__APPLE__)
        sa.__sigaction_u.__sa_handler = systick_clock_signal_handler;
#elif defined(__linux__)
#pragma GCC diagnostic push
#if defined(__clang__)
#pragma clang diagnostic ignored "-Wdisabled-macro-expansion"
#endif
        sa.sa_handler = systick_clock_signal_handler;
#pragma GCC diagnostic pop
#else
#error Platform unsupported
#endif
        sigemptyset (&sa.sa_mask);
        sa.sa_flags = SA_RESTART;

        if (sigaction (clock::signal_number, &sa, nullptr) != 0)
          {
            trace::printf ("port::clock_systick::%s() sigaction() failed\n",
                           __func__);
            abort ();
          }

        // SIGALRM does not queue: when the signal is delayed (host
        // scheduling, timer coalescing on an idle macOS host), the
        // expirations are merged and the delayed ticks are lost, so
        // sysclock runs slower than real time. Not compensated, by
        // decision: each signal processes exactly one tick, as a real
        // tick interrupt does (see the README Limitations).

        // The tick period in microseconds (rounded down when the
        // frequency does not divide 1 MHz); setitimer() requires tv_usec
        // below 1000000, and a zero period would disarm the timer.
        static_assert (rtos::clock_systick::frequency_hz > 0
                           && rtos::clock_systick::frequency_hz <= 1000000,
                       "OS_INTEGER_SYSTICK_FREQUENCY_HZ must be 1 to 1000000");
        constexpr uint32_t period_us
            = 1000000 / rtos::clock_systick::frequency_hz;

        /* struct */ itimerval tv{};
        tv.it_value.tv_sec = period_us / 1000000;
        tv.it_value.tv_usec = period_us % 1000000;
        tv.it_interval = tv.it_value;

        if (setitimer (ITIMER_REAL, &tv, nullptr) != 0)
          {
            trace::printf ("port::clock_systick::%s() setitimer() failed\n",
                           __func__);
            abort ();
          }
      }

      // ======================================================================

      // The time of the previous tick, in microseconds, as accounted
      // by the core (see cycles_per_tick()).
      static uint64_t previous_timestamp;

      // The microseconds since an unspecified start, from the monotonic
      // clock, which is not affected by changes of the system time.
      static uint64_t
      get_current_micros (void)
      {
        /* struct */ timespec ts;
        clock_gettime (CLOCK_MONOTONIC, &ts);

        return static_cast<uint64_t> (ts.tv_sec) * 1000000u
               + static_cast<uint64_t> (ts.tv_nsec) / 1000u;
      }

      // The core adds the uint32_t per tick deltas to its count; a longer
      // gap (the process stopped by a debugger, the host asleep) is
      // returned in parts over the next ticks, so no time is lost and
      // hrclock stays monotonic.
      static uint32_t
      micros_since_previous (void)
      {
        uint64_t delta = get_current_micros () - previous_timestamp;

        return (delta > UINT32_MAX) ? UINT32_MAX
                                    : static_cast<uint32_t> (delta);
      }

      void
      clock_highres::start (void)
      {
        previous_timestamp = get_current_micros ();
      }

      uint32_t
      clock_highres::input_clock_frequency_hz (void)
      {
        // The monotonic clock is read in microseconds; a higher frequency
        // would change the meaning of the hrclock values.
        return 1000000;
      }

      uint32_t
      clock_highres::cycles_per_tick (void)
      {
        uint32_t delta = micros_since_previous ();
        previous_timestamp += delta;

        return delta;
      }

      uint32_t
      clock_highres::cycles_since_tick (void)
      {
        return micros_since_previous ();
      }

    } /* namespace port */
  } /* namespace rtos */
} /* namespace os */

// ----------------------------------------------------------------------------

#endif /* defined(__APPLE__) || defined(__linux__) */
