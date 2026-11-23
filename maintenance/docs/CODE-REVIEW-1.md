# Code review 1 - µOS++ IIIe synthetic POSIX port

Scope: the whole port, branch `xpack-development` at 9220598 (version
1.0.1, plus the two fixes made during the Cortex-M port review):
`CMakeLists.txt`, `include/cmsis-plus/rtos/port/os-c-decls.h`,
`os-decls.h`, `os-inlines.h`, `src/rtos/os-core.cpp`,
`src/diag/trace-posix.cpp`, the packaging (`package.json`,
`.npmignore`) and the user documentation (`README.md`, `CLAUDE.md`).
`maintenance/` was not reviewed.

Reviewed together with the portable core (`micro-os-plus-iii`,
`xpack-development` at ef65a77e, version 7.2.0 in preparation), with
which the port is compiled, and compared with the Cortex-M port
(`micro-os-plus-iii-cortexm`, `xpack-development`, after its review).

Method: reading the code against the port contract (`os::rtos::port`
in the core `os-decls.h`), the POSIX specifications of `ucontext`,
signals and timers, and the core code that calls the port; plus an
experiment on macOS (native clang debug build of the core `rtos-apis`
test, temporary code, removed afterwards) and `npm pack --dry-run` of
both ports. While fixing, more temporary experiments are quoted in the
sections that use them, and GNU/Linux was checked on an AArch64 host.

Outcome (2026-11-24): all findings are addressed: #1, #2, #4 and #5
fixed in issues #4, #5, #6 and #3 (commits listed in each section),
with a test in the core where the problem can be reproduced; #6 to #10
fixed without issues; #3 decided: the lost ticks are documented, not
compensated. GNU/Linux checked on Ubuntu 24.04, AArch64, GCC 13.3.

## Summary

| # | Severity | File | Issue |
| --- | --- | --- | --- |
| #1 | narrow | `src/rtos/os-core.cpp:64` | New threads inherit the signal mask of the code that creates them; a thread created with SIGALRM blocked runs without ticks (**fixed**) |
| #2 | minor | `src/rtos/os-core.cpp:525` | `hrclock` uses `gettimeofday()` (wall clock): it jumps when the system time is changed; deltas wrap after 71 minutes; overflow with a 32-bit `time_t` (**fixed**) |
| #3 | decision | `src/rtos/os-core.cpp:494` | SIGALRM does not queue: ticks are lost when the process is not scheduled or the signal is blocked longer than a tick; `sysclock` falls behind real time (decided: not compensated, documented in ee557c8) |
| #4 | minor | `include/cmsis-plus/rtos/port/os-inlines.h:97` | The idle `pause()` can miss a wake-up: a signal between the idle check and `pause()` delays the woken thread by one tick (**fixed**) |
| #5 | narrow | `src/rtos/os-core.cpp:482` | The timer period is computed only in microseconds: 1 Hz fails (`tv_usec` out of range), above 1 MHz the timer is silently disarmed (**fixed**) |
| #6 | minor | `.npmignore` | The npm package ships `CLAUDE.md` and `maintenance/`; the Cortex-M port ships `maintenance/` too (**fixed**) |
| #7 | nit | `src/rtos/os-core.cpp:218` | `start()` skips the first-thread bookkeeping that the Cortex-M port now does (instrumentation; the CPU cycles timestamp was a wrong claim) (**fixed**) |
| #8 | docs | `src/rtos/os-core.cpp:31` | `signal_nesting`: a plain `uint32_t` changed in a signal handler, with a generic C name; in fact a port interface (documented, dead branch removed; **fixed**) |
| #9 | nit | `include/cmsis-plus/rtos/port/os-c-decls.h:34`, `src/rtos/os-core.cpp:91` | `_XOPEN_SOURCE` required also with `libucontext`; `makecontext()` receives a pointer as an `int` argument (**fixed**) |
| #10 | docs | `README.md`, `CLAUDE.md`, headers | Outdated or wrong statements; limitations not documented (**fixed**) |

Severity values:

- **important**: a real bug with visible effects in normal use;
- **pre-existing**: a defect or risk that does not show today;
- **narrow**: a real bug, but only in uncommon configurations;
- **minor**: a real defect with a small window or impact;
- **decision**: a design choice or missing feature, needs a decision
  before fixing;
- **nit**, **docs**: style, comments, documentation.

All findings are already present in 1.0.1; none was introduced by a
recent change.

## Suggested fix order

Phase 1, small and contained:

1. #6 - `.npmignore` in both ports (before the next release) (**fixed**,
   1b4b204; Cortex-M d1d38d0).
2. #5 - the timer period (**fixed**, df0207f, issue #3).
3. #8, #9, #10 - nits and documentation (#8 **fixed**, b8be14d; #9 **fixed**, 17db682; #10 **fixed**, 3788112).
4. #7 - the first-thread bookkeeping, as in the Cortex-M port (**fixed**,
   3802953).

Phase 2, behaviour, with tests:

1. #1 - clear SIGALRM in the new thread's signal mask (deterministic
   test available) (**fixed**, d677edb, issue #4).
2. #2 - `CLOCK_MONOTONIC` for `hrclock` (**fixed**, fb05566, issue #5).
3. #4 - `sigsuspend()` in `wait_for_interrupt()` (**fixed**, e21e63d, issue #6).

Phase 3, decide first:

1. #3 - compensate the lost ticks, or document the behaviour (decided:
   documented only, ee557c8).

---

## #1 - New threads inherit the creator's signal mask

**File:** `src/rtos/os-core.cpp:64` (`context::create()`)

**Status:** fixed in d677edb (issue #4; test in the core a90f8b74): the clock
signal is removed from the new context's saved mask.

### Problem

`context::create()` initialises the new context with `getcontext()`
and then `makecontext()`. `getcontext()` also saves the current signal
mask, and `setcontext()`/`swapcontext()` restore it, so a new thread
starts with the signal mask of the code that created it.

The port implements the interrupt critical sections by blocking
SIGALRM. If a thread is created while SIGALRM is blocked (inside an
`interrupts::critical_section`, or from code that blocked it for other
reasons), the new thread starts with the clock signal blocked; every
critical section it enters returns "blocked" and restores "blocked",
so the thread runs its whole life without ticks. They are delivered
only when another thread, with an unblocked mask, runs.

On Cortex-M the interrupt masks (PRIMASK, BASEPRI) are not part of the
thread context; a new thread always runs with interrupts enabled.

### Scenario

Experiment (macOS, native clang debug, temporary code in the core
`rtos-apis` test): a thread busy-waits for 5 `sysclock` ticks, with a
2 seconds `hrclock` limit:

- created normally: "5 ticks in 5088 us";
- created inside an `interrupts::critical_section`: "0 ticks in
  2000003 us".

A thread that sleeps is not affected in practice, since the idle
thread (unblocked) receives the signals meanwhile; a thread that polls
the clock, or a long running one, is.

### Documentation and tests

The documentation does not say that creating a thread inside a
critical section is forbidden; the core creates threads inside a
scheduler critical section only (not an interrupts one), so the
problem does not show in the tests.

### Port behaviour

Cortex-M: not affected (see above).

### Suggested fix

After `getcontext()`, remove the clock signal from the saved mask
(`uc_sigmask` for the system `ucontext`, `uc_sigmask_posix` for
`libucontext`), so every thread starts with the clock signal enabled,
as on Cortex-M. Deterministic test: the experiment above, in the core
`rtos-apis` test (it is portable; on Cortex-M the thread runs after
the critical section exits).

### Analysis before fixing

- `libucontext`'s POSIX variants handle the mask as the system ones:
  `libucontext_posix_getcontext()` saves it with `sigprocmask()` in
  `uc_sigmask_posix`, `setcontext()`/`swapcontext()` restore it from
  there (`libucontext_posix.c`); the field name differs from the system
  `uc_sigmask`;
- only SIGALRM must change: the CMSIS validator manages SIGUSR1 itself
  (it blocks it at initialisation and blocks/unblocks it to disable and
  enable its "IRQ"), so the rest of the mask must still be inherited.

### Fix applied

- `os-c-decls.h`: `os_impl_uc_sigmask` (`uc_sigmask_posix` with
  `libucontext`, `uc_sigmask` with the system `ucontext`), next to the
  other `os_impl_*` macros;
- `context::create()`: after `getcontext()`,
  `sigdelset (&ctx->os_impl_uc_sigmask, clock::signal_number)`, with a
  comment; the other signals keep the creator's mask.

No change for threads created normally; no change to the core, the C
API or the data layout.

### Verification

- new test in the core `test-cpp-api.cpp` ("Threads"), portable: a
  thread created inside an `interrupts::critical_section` busy-waits for
  5 `sysclock` ticks, at most 1 second (`hrclock`, in its own units);
  the test fails if fewer than 5 ticks were seen;
- negative check, native clang debug with the previous port
  `os-core.cpp`: "thread created in a critical section saw 0 ticks",
  the test fails; with the fix it passes;
- native clang and GCC, QEMU M0, M3, M4F and M7F suites (debug and
  release) pass, with no warnings.
- GNU/Linux (checked on 2026-11-24 on Ubuntu 24.04, AArch64, GCC 13.3,
  glibc, `libucontext` AArch64; the core `rtos-apis` test built with the
  system GCC from the same sources and definitions as the native
  configuration): debug and release build with no warnings and pass,
  including "thread created in a critical section ok".

---

## #2 - `hrclock` uses the wall clock

**File:** `src/rtos/os-core.cpp:525` (`get_current_micros()`)

**Status:** fixed in fb05566 (issue #5): `CLOCK_MONOTONIC`, 64-bit arithmetic,
per tick deltas clamped and carried over.

### Problem

`get_current_micros()` uses `gettimeofday()`, which returns the wall
clock (`CLOCK_REALTIME`). `cycles_per_tick()` and `cycles_since_tick()`
return the difference from the previous tick as a `uint32_t` number
of microseconds, which the core adds to `steady_count_`:

- when the system time is changed (manually, or stepped by NTP), the
  difference is negative and wraps to about 4295 seconds: `hrclock`
  jumps forward by 71 minutes; a forward step makes it jump forward by
  the step;
- a real gap of more than 2^32 µs (71.6 minutes) between two ticks
  (the process stopped by a debugger or `SIGSTOP`, a laptop asleep)
  also wraps and loses multiples of 71.6 minutes;
- `tp.tv_sec * 1000000 + tp.tv_usec` is computed in `time_t`; with a
  32-bit `time_t` (32-bit Linux hosts) it overflows (signed overflow,
  undefined behaviour).

`hrclock` is meant to be monotonic (it is the base of the CPU cycles
statistics and of `hrclock` timeouts).

### Scenario

Run a test, change the system time backwards: the next tick adds about
4295 seconds to `hrclock`.

### Documentation and tests

Not covered; the core tests check that `hrclock` does not go backwards
(Cortex-M #2), not that it ignores system time changes.

### Port behaviour

Cortex-M: `hrclock` is derived from SysTick and is not affected.

### Suggested fix

Use `clock_gettime (CLOCK_MONOTONIC, ...)` (macOS 10.12 and later,
Linux), compute in `uint64_t`, and keep the per tick difference in 64
bits until it is added (or limit it, see #3). Regression check only:
the system time cannot be changed from the tests.

### Analysis before fixing

- with the test definitions (`_POSIX_C_SOURCE=200809L`,
  `_XOPEN_SOURCE=700L`), `clock_gettime (CLOCK_MONOTONIC)` compiles and
  works on macOS 15 (1 µs resolution); on macOS the "native-cmake-gcc"
  configuration also uses Apple clang (`/usr/bin/g++`);
- the core contract (`os-decls.h`) has `cycles_per_tick()` and
  `cycles_since_tick()` return `uint32_t`, and the core adds the tick
  delta to `steady_count_`; the port cannot return more than 2^32 µs
  per tick, but it can carry the rest over to the next ticks, by
  advancing the previous timestamp only by the amount returned; the
  result stays monotonic, also while catching up;
- the units stay 1 MHz: nanoseconds would change the meaning of every
  `hrclock` value for applications;
- `cycles_since_tick()` is called by the core only inside the
  `hrclock::now()` critical section, and the previous timestamp changes
  only in the tick handler.

### Fix applied

- `get_current_micros()`: `clock_gettime (CLOCK_MONOTONIC)`, computed in
  `uint64_t` (no `time_t` overflow);
- `micros_since_previous()`: the difference from the previous tick,
  clamped to `UINT32_MAX`; `cycles_per_tick()` advances the previous
  timestamp by the returned amount, so a longer gap is returned in parts
  over the next ticks; `cycles_since_tick()` returns the clamped
  difference;
- `#include <time.h>`; comments.

Behaviour change: `hrclock` no longer follows changes of the system
time (the intent). No change to the core, the C API or the data
layout.

### Verification

- experiment, native clang debug, a temporary offset added to the time
  source after the start, `hrclock` measured across 5 ticks, then
  removed:
  - previous code, 10 s backward step: `hrclock` advanced 4284972 ms
    (about 71 minutes); not possible with a monotonic source;
  - previous code, 2 hours forward gap: advanced 2905037 ms (71.6
    minutes lost);
  - fixed code, 2 hours forward gap: advanced 7200005 ms;
- native clang and GCC suites (debug and release) pass, with no
  warnings, including the core test "hrclock monotonic ok".
- GNU/Linux (checked on 2026-11-24 on Ubuntu 24.04, AArch64, GCC 13.3,
  glibc, `libucontext` AArch64; the core `rtos-apis` test built with the
  system GCC from the same sources and definitions as the native
  configuration): debug and release build with no warnings and pass,
  including "hrclock monotonic ok".
  The binary uses `clock_gettime@GLIBC_2.17`.

---

## #3 - Ticks are lost when SIGALRM coalesces

**File:** `src/rtos/os-core.cpp:494` (`clock_systick::start()`),
`:412` (`systick_clock_signal_handler()`)

**Status:** decided: not compensated; documented in ee557c8 (no issue).

### Problem

The tick is `setitimer (ITIMER_REAL)` delivering SIGALRM. Standard
signals are not queued: if the timer expires again while SIGALRM is
pending (the process not scheduled by the host, or the signal blocked
by a critical section longer than a tick), the expirations merge and
the handler runs once. Each run counts one tick, so `sysclock` falls
behind real time on a loaded host; timeouts in ticks last longer than
requested, and `sysclock` and `hrclock` (which measures the real
interval, #2) diverge.

On Cortex-M the same happens only with critical sections longer than a
tick (the SysTick pending bit also coalesces); on a host the process
can be descheduled at any time, outside the application's control.

### Suggested fix

A decision is needed:

- compensate: in the handler, compute the number of tick periods
  elapsed since the previous handled tick (from a monotonic clock) and
  call `os_systick_handler()` that many times (bounded), so `sysclock`
  follows real time;
- or keep the simulation semantics (one tick per handler run) and
  document that, on a synthetic platform, tick based timeouts are a
  lower bound.

### Analysis before deciding

Measured with a temporary test (2000 ticks of `sleep_for()`, timed with
`hrclock`, release builds), then removed:

- idle macOS host: 2173.1 ms, about 8% of the ticks lost (timer
  coalescing of an idle system);
- the same Mac with 24 busy processes on 12 CPUs: 2000.1 ms, no loss;
- GNU/Linux (ampere, Ubuntu 24.04, AArch64, 4 CPUs, idle): 2000.0,
  2000.0 and 2001.0 ms, no loss.

Without compensation: tick based delays and periodic timers run up to
about 8% slow on an idle Mac (still correct for the API, whose timeouts
are lower bounds); `rtclock`, simulated from the tick when there is no
RTC driver (`os_systick_handler()`, one second every `frequency_hz`
ticks), drifts too; `sysclock` and `hrclock` disagree; the timing
differs from run to run. With compensation: the ticks of a delayed
signal would be processed in a burst, inside one signal handler, and
the port would be more complex.

### Decision

Not compensated: each signal processes exactly one tick, as a real tick
interrupt does; the POSIX port is mainly a development and test
platform, where "at least N ticks" is enough.

### Documentation

- `README.md` Limitations: the lost ticks, the clocks they affect
  (`sysclock`, timeouts, timers, `rtclock`; not `hrclock`), the
  measured values, and that they are not compensated;
- `CLAUDE.md`: the same, in short; also the high resolution clock line,
  outdated since #2 (`CLOCK_MONOTONIC`);
- `os-core.cpp`: a comment in `clock_systick::start()`.

No code change (comments only).

---

## #4 - The idle `pause()` can miss a wake-up

**File:** `include/cmsis-plus/rtos/port/os-inlines.h:97`
(`scheduler::wait_for_interrupt()`)

**Status:** fixed in e21e63d (issue #6): the ready list is checked with all
signals blocked, then `sigsuspend()`.

### Problem

The idle thread calls `wait_for_interrupt()` after finding no other
thread ready; the port calls `pause()`. If SIGALRM arrives after the
check and before `pause()`, the handler readies a thread (it cannot
switch context, the port is cooperative), and `pause()` then waits for
the next signal: the woken thread starts one tick late.

On Cortex-M the same window exists before `WFI`, but the interrupt
also pends PendSV, which switches to the woken thread before `WFI`
executes.

### Suggested fix

The usual POSIX pattern: block SIGALRM, check that no thread other
than the current one is ready (`ready_threads_list_` empty), and wait
with `sigsuspend()` on the mask without SIGALRM, which unblocks and
waits atomically; restore the mask. Not testable deterministically
(the window is a few instructions); regression only.

### Analysis before fixing

The review understated the window and the signals involved:

- the core idle loop runs `os_rtos_idle_actions()` (which first
  destroys the terminated threads, possibly for a while, and then calls
  `wait_for_interrupt()`), and only afterwards checks
  `ready_threads_list_` to yield; a thread made ready by a signal
  anywhere in that stretch made `pause()` wait for the next signal;
- not only SIGALRM makes threads ready: the CMSIS validator's SIGUSR1
  handler, and any application that simulates interrupts with signals,
  do too; blocking only SIGALRM during the check would leave them
  racing;
- on Cortex-M the interrupt pends PendSV, which preempts idle before
  `WFI`; on POSIX the handler cannot switch, so idle must notice.

### Fix applied

- `wait_for_interrupt()`: block all signals, check
  `rtos::scheduler::ready_threads_list_.empty()`, and only then wait
  with `sigsuspend()` on the previous mask (which unblocks and waits
  atomically, keeping the signals the application blocked on purpose);
  restore the mask; a comment explains why.

No change to the core, the C API or the data layout.

### Verification

- experiment, native clang debug, the window widened with a temporary
  3 ms busy delay in the idle wait (before `pause()` in the previous
  code, before the signals are blocked in the fixed code), then
  removed; 100 x `sleep_for(1)` measured with `hrclock`:
  - no delay: 100.0 ms (previous), 100.1 ms (fixed);
  - 3 ms delay: 386.2 ms (previous; 0.86 ms per iteration waiting for
    the next tick), 302.5 ms (fixed; only the delay);
- GNU/Linux (ampere, Ubuntu 24.04, AArch64, GCC 13.3): the core
  `rtos-apis` test, debug and release, no warnings, passes;
- native clang and GCC suites on macOS (debug and release) pass, with
  no warnings.

---

## #5 - The timer period is computed only in microseconds

**File:** `src/rtos/os-core.cpp:482` (`clock_systick::start()`)

**Status:** fixed in df0207f (issue #3): the period is split into seconds and
microseconds; the frequency is checked at compile time.

### Problem

`tv_usec = 1000000 / frequency_hz` with `tv_sec = 0`:

- with `OS_INTEGER_SYSTICK_FREQUENCY_HZ` 1, `tv_usec` is 1000000,
  outside the valid range; `setitimer()` fails with `EINVAL` and the
  port aborts with "setitimer() failed", although 1 Hz is a valid
  configuration;
- above 1 MHz, `tv_usec` is 0; an `it_value` of 0 disarms the timer, so
  `setitimer()` succeeds and no tick ever arrives (the same class as
  Cortex-M #9).

### Suggested fix

Compute the period in microseconds, stop with a message if it is 0,
and split it into `tv_sec` and `tv_usec`. Checked by building with the
two frequencies (manual check, not a permanent test).

### Analysis before fixing

- `clock_systick::frequency_hz` is `static constexpr uint32_t` (core
  `os-clocks.h`), so on POSIX the frequency can be checked at compile
  time (on Cortex-M #9 the check depends on `SystemCoreClock`, known
  only at run time);
- the review overstated the case above 1 MHz: with the current core
  such a build does not compile anyway (`src/libcpp/chrono.cpp` fails
  for sub-microsecond tick durations), so the timer was never silently
  disarmed in practice; the 1 Hz case is real;
- the same function had dead debug blocks (`#if 1 ... #else`,
  `#if 0`) and a platform-specific clear (`memset`/`timerclear`).

### Fix applied

- `static_assert` that `frequency_hz` is 1 to 1000000, with a message;
- the period in microseconds, split into `tv_sec` and `tv_usec`;
  `it_interval` equal to `it_value`; `itimerval tv{}` instead of the
  `memset`/`timerclear` split;
- the dead debug blocks removed; a comment on the rounding when the
  frequency does not divide 1 MHz (as on Cortex-M, not rejected).

No change for the valid frequencies of 2 Hz to 1 MHz; no change to the
core, the C API or the data layout.

### Verification

- manual checks, native clang debug, temporary
  `OS_INTEGER_SYSTICK_FREQUENCY_HZ` in the test configuration, then
  restored:
  - 1 Hz: before, "setitimer() failed" and abort; after, the scheduler
    starts and the test runs (slowly, with one-second ticks);
  - 2 MHz: before, the build fails only in the core `chrono.cpp`;
    after, also with the port's message "OS_INTEGER_SYSTICK_FREQUENCY_HZ
    must be 1 to 1000000";
- native clang and GCC suites (debug and release) pass, with no
  warnings.

---

## #6 - The npm package ships `CLAUDE.md` and `maintenance/`

**File:** `.npmignore`

**Status:** fixed in 1b4b204 (no issue) and, in the Cortex-M port, in
d1d38d0.

### Problem

`npm pack --dry-run` lists, besides the library files, `CLAUDE.md` and
`maintenance/docs/preemption-and-smp-plan.md` (`README-MAINTAINER.md`
and `NOTES.md` are excluded by older patterns). The core's
`.npmignore` excludes `.claude/`, `CLAUDE.md` and `maintenance/`,
which are not part of the library.

The Cortex-M port has the same gap: its package would ship
`maintenance/docs/CODE-REVIEW-1.md`.

### Suggested fix

Add `.claude/`, `CLAUDE.md` and `maintenance/` to the `.npmignore` of
both ports, as in the core; check with `npm pack --dry-run`.

### Analysis before fixing

- neither port has a `files` list in `package.json`, so `.npmignore`
  decides; the core has a "Claude Code" block (`.claude/`, `CLAUDE.md`)
  and `maintenance/`, the ports have none of them;
- the POSIX port has `CLAUDE.md` and `maintenance/`, the Cortex-M port
  only `maintenance/`; nothing in `maintenance/` is used by the `xpm`
  actions or by `CMakeLists.txt`.

### Fix applied

- both ports: the core's "Claude Code" block (`.claude/`, `CLAUDE.md`)
  after "Visual Studio Code", and `maintenance/`, as in the core
  (`.claude/` also in the Cortex-M port, to keep the three files alike).

No code change.

### Verification

`npm pack --dry-run` before and after:

- POSIX port: `CLAUDE.md`, `maintenance/docs/CODE-REVIEW-1.md` and
  `maintenance/docs/preemption-and-smp-plan.md` no longer listed;
- Cortex-M port: `maintenance/docs/CODE-REVIEW-1.md` no longer listed;
- both now list only `CHANGELOG.md`, `CMakeLists.txt`, `LICENSE`,
  `README.md`, `package.json`, `include/...` and `src/...`.

---

## #7 - `start()` skips the first-thread bookkeeping

**File:** `src/rtos/os-core.cpp:218` (`scheduler::start()`)

**Status:** fixed in 3802953 (no issue): the instrumentation `active` event
for the first thread.

### Problem

`start()` selects the first thread with `unlink_head()` and enters it
with `setcontext()`, without the bookkeeping that the core's
`internal_switch_threads()` does for later switches. Since the
Cortex-M fix e6062f2 (micro-os-plus/micro-os-plus-iii-cortexm#8),
which took this port as a model, the Cortex-M `start()` also emits the
instrumentation `active` event for the first thread and, with
`OS_INCLUDE_RTOS_STATISTICS_THREAD_CPU_CYCLES`, sets the switch
timestamp, so the time before the start is not charged to the first
thread. This port does neither, so the two ports now differ.

### Suggested fix

Do the same as the Cortex-M port, before `setcontext()`.

### Analysis before fixing

The review was partly wrong:

- the CPU cycles timestamp: the core's `scheduler::start()` already
  resets `cpu_cycles_` and sets `switch_timestamp_ = hrclock.now()`
  right before calling `port::scheduler::start()`, so the time before
  the start was never charged to the first thread, in either port;
  the assignment added to the Cortex-M `start()` in its #8 (e6062f2)
  repeats it a few microseconds later and is redundant; removed in the
  Cortex-M port (c244fa1);
- the real difference is only `instrumentation::thread::active()` for
  the first thread, which the Cortex-M `start()` emits (as the core's
  `internal_switch_threads()` does for the next switches) and this port
  did not.

Severity changed to nit: consistency between the ports, no effect on
scheduling.

### Fix applied

- `start()`: `instrumentation::thread::active (current_thread_)` right
  after `unlink_head()`, inside the existing critical section, with a
  comment (the statistics and the timestamp are set by the core).

No change to the core, the C API or the data layout.

### Verification

- not observable in the tests (the instrumentation hooks are no-ops on
  the native platform), and not testable yet with the current SEGGER
  SystemView support;
- native clang and GCC suites (debug and release) pass, with no
  warnings.

---

## #8 - `signal_nesting`

**File:** `src/rtos/os-core.cpp:31`, `include/cmsis-plus/rtos/port/os-inlines.h:60`

**Status:** fixed in b8be14d (no issue): documented as a port interface; the
dead branch removed; name and type unchanged (see below).

### Problem

- `signal_nesting` is changed in the signal handler and read by
  `in_handler_mode()`; it is a plain `uint32_t`, not
  `volatile sig_atomic_t`. It works because the reads are not in tight
  loops, but it is not the type POSIX specifies for this use.
- it is a global with C linkage and a generic name, visible to the
  application (possible clashes).
- the handler has a branch for other signals that writes `?` to
  standard output; the handler is installed only for SIGALRM, so it is
  dead code.

### Suggested fix

`volatile sig_atomic_t`, with a prefixed name (for example
`os_port_signal_nesting`); drop the dead branch.

### Analysis before fixing

The review was wrong on the name and the type:

- `signal_nesting` is used outside the port: the CMSIS RTOS validator
  (`arm-cmsis-rtos-validator.git`, `cmsis-plus/src/main.c`, used by the
  core tests) simulates interrupts with SIGUSR1 and declares
  `extern uint32_t signal_nesting;`, incrementing it around its test
  handlers, so the RTOS sees them in handler mode. With C linkage and
  `uint32_t`, it is in practice the interface for any code that
  simulates interrupts with signals; renaming it breaks the validator
  (and applications that copied the pattern), changing the type
  (`sig_atomic_t` is a signed `int`) contradicts its declaration;
- the type is not a real problem: `uint32_t` loads and stores are atomic
  on the supported hosts, the only reader (`in_handler_mode()`) is not
  in a tight loop, and nested handlers restore the value before the
  outer one stores it;
- the `?` branch is dead (the handler is installed only for SIGALRM)
  and wrote to standard output even without tracing.

Severity changed to docs.

### Fix applied

- `os-inlines.h`: a comment on `signal_nesting`: the handler nesting
  level behind `in_handler_mode()`, part of the port interface, to be
  incremented by code that simulates other interrupts with signals (as
  the validator does); changed only in signal handlers;
- `os-core.cpp`: the dead branch replaced by
  `assert (signum == clock::signal_number)`;
- the README part goes with #10.

No change to the symbol, the validator, the core, the C API or the
data layout.

### Verification

- native clang and GCC suites (debug and release, including the CMSIS
  validator, which uses the counter from its SIGUSR1 handlers) pass,
  with no warnings.

---

## #9 - `_XOPEN_SOURCE` and `makecontext()` arguments

**File:** `include/cmsis-plus/rtos/port/os-c-decls.h:34`,
`src/rtos/os-core.cpp:91`

**Status:** fixed in 17db682 (no issue): the check moved to the system
`ucontext` branch; the `makecontext()` assumption documented.

### Problem

- `os-c-decls.h` stops with an `#error` if `_XOPEN_SOURCE` is not
  defined, before testing `OS_INCLUDE_LIBUCONTEXT`; `_XOPEN_SOURCE` is
  needed only by the system `<ucontext.h>` (on macOS it also hides some
  BSD APIs).
- `makecontext (ctx, func, 1, args)` passes a pointer; POSIX defines
  the extra arguments as `int`. It works with the implementations in
  use (glibc on x86_64 and AArch64, `libucontext`), which pass full
  registers, but it is not portable.

### Suggested fix

Move the check into the system `ucontext` branch; document the
`makecontext()` assumption next to the call.

### Analysis before fixing

- the core tests always use `libucontext`: every test application
  defines `OS_INCLUDE_LIBUCONTEXT` in its `os-app-config.h` (native
  branch), and the native binaries reference only `libucontext_*`
  context functions; `_XOPEN_SOURCE=700L` comes from the `libucontext`
  xPack, which adds it as an interface definition to the targets that
  link it (the native platform defines only `_POSIX_C_SOURCE`). So the
  misplaced check had no visible effect; the fix is for correctness and
  a clearer message;
- `libucontext` reads the `makecontext()` arguments at register width
  on all its architectures (`long long` on x86_64, `unsigned long` on
  AArch64 and Arm), and so does glibc on x86_64 and AArch64; passing the
  thread pointer is safe with every implementation in use, so only the
  assumption needs documenting (splitting the pointer into two `int`
  and adding a trampoline would cost code at every thread start for no
  practical gain);
- not noticed by the review (for #10): the system `ucontext` path is
  the default but is not tested, and the README does not mention
  `OS_INCLUDE_LIBUCONTEXT`.

### Fix applied

- `os-c-decls.h`: the `_XOPEN_SOURCE` check moved into the system
  `ucontext` branch, with the message "The system ucontext requires
  _XOPEN_SOURCE=600L or 700L; alternatively use libucontext
  (OS_INCLUDE_LIBUCONTEXT).";
- `os-core.cpp`: a comment before `makecontext()` documenting the
  register-width assumption.

No behaviour change; no change to the core, the C API or the data
layout.

### Verification

- manual compiles: `os-c-decls.h` alone with the system `ucontext` and
  no `_XOPEN_SOURCE` stops with the new message; with `_XOPEN_SOURCE` it
  compiles; with `OS_INCLUDE_LIBUCONTEXT` and no `_XOPEN_SOURCE` it
  compiles, and so does the whole port `os-core.cpp`;
- release binaries hashed with the previous and the new port files
  (`hash-builds.sh`, native and QEMU): all identical;
- native clang and GCC suites (debug and release) pass, with no
  warnings.

---

## #10 - Documentation

**Status:** fixed in 3788112 (no issue): header comments, `README.md`,
`CLAUDE.md`; the `tests/trace-*` programs kept.

- `os-inlines.h` header comment: "If contains the scheduler
  implementation..." and "included in all src/os-*.cpp files"; it
  contains the inline port functions and is included at the end of
  `rtos/os.h` (the same as Cortex-M #12). `os-decls.h` and
  `os-c-decls.h` also name the wrong including files. `os-core.cpp`
  includes `port/os-inlines.h` again after `os.h`.
- `README.md`:
  - "compiled with GCC 11, clang 12, clang 13 arm-none-eabi-gcc 11"
    (the port is never compiled with arm-none-eabi-gcc; the tests use
    GCC 11-14 and clang 13-19);
  - "Dependencies: none" (the portable core; optionally `libucontext`,
    with `OS_INCLUDE_LIBUCONTEXT`);
  - the CMake example ends with a stray backtick;
  - "Status: ... end-of-life" (the Cortex-M README was aligned with the
    core in Cortex-M #12);
  - "Limitations" mentions only the cooperative scheduler; not
    documented: the tick, timer callbacks and other "interrupt" code
    run inside a signal handler (only async-signal-safe functions may
    be called there; see the core issue #139), the signal frames use
    the thread stacks, and the timing is that of a host process (#3);
  - "Tests: TBD" (the core tests, native platform).
- `CLAUDE.md`: `test/` (renamed to `tests/`), `README-MAINTAINER.md`
  and `NOTES.md` (moved to `maintenance/docs/`), the end-of-life
  status, and the `.npmignore` description (#6).
- `tests/trace-stdout`, `tests/trace-stderr`: small `main.cpp` files
  not built by anything; either add them to the core tests or remove
  them.

### Analysis before fixing

- the header "included from" lines were checked against the actual
  includes: on POSIX `os-c-decls.h` is included only from the port
  `os-decls.h` (the core `exception-handlers.c` is Cortex-M only);
- the README install section is fine (it already links to
  `maintenance/docs/README-MAINTAINER.md`; the version tag is updated at
  release);
- found while fixing #8 and #9: `signal_nesting` is part of the port
  interface; the tests always use `libucontext`
  (`OS_INCLUDE_LIBUCONTEXT`), so the system `ucontext` path is not
  tested, and the README did not mention `OS_INCLUDE_LIBUCONTEXT`;
- `CLAUDE.md` also described the old `_XOPEN_SOURCE` check (#9) and the
  old `.npmignore` (#6).

### Fix applied

- headers: `os-inlines.h` describes its content and where it is
  included; `os-decls.h` and `os-c-decls.h` name the files that include
  them; `os-decls.h`: the "Platform definitions" comment replaced;
  `os-core.cpp`: the redundant include of `port/os-inlines.h` removed;
- `README.md`: status aligned with the core (fully functional, IVe in
  progress); compilers (GCC 11-14, clang 13-19); preprocessor
  definitions (`OS_INCLUDE_LIBUCONTEXT` recommended and tested,
  otherwise `_XOPEN_SOURCE` for the untested system `ucontext`);
  dependencies (the portable part, `libucontext`); the stray backtick;
  a Limitations section (cooperative scheduler, interrupts simulated
  with signals and the async-signal-safe rule, `signal_nesting`, signal
  frames on the thread stacks, host process timing); Tests (the core
  tests, native platform; the `tests/trace-*` programs mentioned as not
  built);
- `CLAUDE.md`: status, `tests/`, `maintenance/docs/` paths, the
  `_XOPEN_SOURCE` check, `OS_INCLUDE_LIBUCONTEXT` in the tests,
  `signal_nesting` as an interface, `.npmignore`;
- the `tests/trace-*` programs are kept (decision).

### Verification

- release binaries hashed before and after (`hash-builds.sh`, native and
  QEMU): all identical.
