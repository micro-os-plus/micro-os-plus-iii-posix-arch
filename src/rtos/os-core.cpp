/*
 * This file is part of the µOS++ distribution.
 *   (https://github.com/micro-os-plus)
 * Copyright (c) 2016-2023 Liviu Ionescu. All rights reserved.
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
#include <cmsis-plus/rtos/port/os-inlines.h>

#include <sys/time.h>

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
        /* class */ rtos::thread::context* th_ctx =
            static_cast</* class */rtos::thread::context*> (context);
        memset (&th_ctx->port_, 0, sizeof(th_ctx->port_));

#pragma GCC diagnostic push
#if defined(__clang__)
#elif defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wuseless-cast"
#endif
        os_impl_ucontext_t* ctx =
            reinterpret_cast<os_impl_ucontext_t*> (&(th_ctx->port_.ucontext));
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
          return sigismember(&old, clock::signal_number);
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
          return sigismember(&old, clock::signal_number);
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
          sigemptyset(&interrupts::clock_set);

#pragma GCC diagnostic push
#if defined(__clang__)
#elif defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wsign-conversion"
#endif
          sigaddset(&interrupts::clock_set, clock::signal_number);
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

              // Determine the next thread.
              rtos::scheduler::current_thread_ =
                  rtos::scheduler::ready_threads_list_.unlink_head ();
            }

#pragma GCC diagnostic push
#if defined(__clang__)
#elif defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wuseless-cast"
#endif
          os_impl_ucontext_t* new_context =
              reinterpret_cast<os_impl_ucontext_t*> (&(rtos::scheduler::current_thread_->context_.port_.ucontext));
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
          assert(res == 0);
#endif
          abort ();
        }

        // --------------------------------------------------------------------

        state_t
        locked (state_t state)
        {
          os_assert_throw(!interrupts::in_handler_mode (), EPERM);

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
              old_ctx =
                  reinterpret_cast<os_impl_ucontext_t*> (&old_thread->context_.port_.ucontext);
#pragma GCC diagnostic pop

              rtos::scheduler::internal_switch_threads ();

#pragma GCC diagnostic push
#if defined(__clang__)
#elif defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wuseless-cast"
#endif
              new_ctx =
                  reinterpret_cast<os_impl_ucontext_t*> (&rtos::scheduler::current_thread_->context_.port_.ucontext);
#pragma GCC diagnostic pop
            }

          if (old_ctx != new_ctx)
            {
              if (save)
                {
#if defined(OS_TRACE_RTOS_THREAD_CONTEXT)
                  trace::printf (
                      "port::scheduler::%s() swapcontext %s -> %s \n", __func__,
                      old_thread->name (),
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

      // ----------------------------------------------------------------------

      } /* namespace scheduler */

      // ----------------------------------------------------------------------

      static void
      systick_clock_signal_handler (int signum)
      {
#if defined(OS_TRACE_RTOS_SYSCLOCK_TICK)
        trace::printf("{i ");
#endif

        if (signum != clock::signal_number)
          {
            char ce = '?';

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"

            write (1, &ce, 1);

#pragma GCC diagnostic pop

            return;
          }

        signal_nesting++;
        // Call the ticks timer ISR.
        os_systick_handler ();
        signal_nesting--;
#if defined(OS_TRACE_RTOS_SYSCLOCK_TICK)
        trace::printf(" i}");
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
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_RESTART;

        if (sigaction (clock::signal_number, &sa, nullptr) != 0)
          {
            trace::printf ("port::clock_systick::%s() sigaction() failed\n",
                           __func__);
            abort ();
          }

        // set timer
        /* struct */ itimerval tv;
        // first clear all fields
#if defined(__APPLE__)
        memset (&tv, 0, sizeof(tv));
#else
        timerclear(&tv.it_value);
#endif
        // then set the required ones

#if 1
        tv.it_value.tv_sec = 0;
        tv.it_value.tv_usec = 1000000 / rtos::clock_systick::frequency_hz;
        tv.it_interval.tv_sec = 0;
        tv.it_interval.tv_usec = 1000000 / rtos::clock_systick::frequency_hz;
#else
        tv.it_value.tv_sec = 1;
        tv.it_value.tv_usec = 0; //1000000 / rtos::clock_systick::frequency_hz;
        tv.it_interval.tv_sec = 1;
        tv.it_interval.tv_usec = 0;//1000000 / rtos::clock_systick::frequency_hz;
#endif

        if (setitimer (ITIMER_REAL, &tv, nullptr) != 0)
          {
            trace::printf ("port::clock_systick::%s() setitimer() failed\n",
                           __func__);
            abort ();
          }

#if 0
        // Used for initial debugging, to see the signals arriving
        pause ();
        for (int i = 50; i > 0; --i)
          {
            for (int j = 100; j > 0; --j)
              {
                char c = '.';
                write (1, &c, 1);
              }
            char cn = '\n';
            write (1, &cn, 1);
          }
#endif

      }

      // ======================================================================

      static uint64_t previous_timestamp;

      static uint64_t
      get_current_micros (void);

      uint64_t
      get_current_micros (void)
      {
        /* struct */ timeval tp;
        gettimeofday (&tp, nullptr);

        return static_cast<uint64_t> (tp.tv_sec * 1000000 + tp.tv_usec);
      }

      void
      clock_highres::start (void)
      {
        previous_timestamp = get_current_micros ();
      }

      uint32_t
      clock_highres::input_clock_frequency_hz (void)
      {
        // The posix system clock resolution is 1 us, so it makes no
        // sense to assume a frequency higher than 1 MHz.
        return 1000000;
      }

      uint32_t
      clock_highres::cycles_per_tick (void)
      {
        uint64_t ts = get_current_micros ();
        uint32_t delta = static_cast<uint32_t> (ts - previous_timestamp);

        previous_timestamp = ts;

        return delta;
      }

      uint32_t
      clock_highres::cycles_since_tick (void)
      {
        uint64_t ts = get_current_micros ();
        uint32_t delta = static_cast<uint32_t> (ts - previous_timestamp);

        return delta;
      }

    } /* namespace port */
  } /* namespace rtos */
} /* namespace os */

// ----------------------------------------------------------------------------

#endif /* defined(__APPLE__) || defined(__linux__) */
