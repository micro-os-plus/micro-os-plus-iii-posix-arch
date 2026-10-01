# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with
code in this repository.

@.github/copilot-instructions.md

## What this is

The **synthetic POSIX port** of µOS++ IIIe (`@micro-os-plus/micro-os-plus-iii-posix-arch`):
the non-portable part that lets the µOS++ RTOS run as a plain process on
macOS and GNU/Linux. The portable kernel lives in the sibling repo
`../micro-os-plus-iii.git` (`@micro-os-plus/micro-os-plus-iii`); applications
must link both. Other ports with the same shape are
`../micro-os-plus-iii-cortexm.git`, `-riscv`, `-aarch32`, `-aarch64`.

Status: functional but end-of-life since mid-2023 (superseded by µOS++ IVe);
expect maintenance-only changes.

## Layout

- `include/cmsis-plus/rtos/port/` — headers the core includes by fixed path:
  - `os-c-decls.h` — C-compatible types (`os_port_*_t`), pulled in by core
    `cmsis-plus/rtos/os-c-decls.h`. Selects `ucontext` vs `libucontext`
    via `os_impl_*` macros. Errors out if `_XOPEN_SOURCE` is not defined.
  - `os-decls.h` — C++ `os::rtos::port` namespace: stack sizes/magic,
    `interrupts`, `scheduler` lock state, `clock::signal_number`
    (`SIGALRM`), `thread_context_t`. Also defines `OS_INTEGER_*` defaults.
  - `os-inlines.h` — inline port functions (`scheduler::lock/unlock/locked`,
    `wait_for_interrupt` → `pause()`, `in_handler_mode`), included by core
    `cmsis-plus/rtos/os.h`.
- `src/rtos/os-core.cpp` — the actual port implementation.
- `src/diag/trace-posix.cpp` — `os::trace::write()` backend.
- `test/` — tiny trace smoke-test `main.cpp` files; no build scripts here.
- `CMakeLists.txt` — INTERFACE library `micro-os-plus::iii-posix-arch`
  (include dir + the two sources; no defines/options of its own).

The function signatures being implemented are declared in the core repo, in
`../micro-os-plus-iii.git/include/cmsis-plus/rtos/os-decls.h`
(`namespace os::rtos::port`). Check there before changing any signature.

## How the port works

- **Context switching**: `getcontext/makecontext/setcontext/swapcontext`
  (deprecated POSIX, hence `-Wdeprecated-declarations` suppressions).
  `context::create()` builds a ucontext on the thread's µOS++ stack.
  With `OS_INCLUDE_LIBUCONTEXT` the `libucontext_*` equivalents are used
  (the native test platform links `xpack-3rd-party::libucontext`).
- **`scheduler::reschedule()`** must keep the save and restore calls in the
  same function body; do not refactor into helpers/inlines (the comment in the
  code explains why). It uses `swapcontext` when the old thread must be
  resumed later, `setcontext` when it is terminated/destroyed.
- **Cooperative only**: the scheduler is non-preemptive; context switches
  never happen from the signal handler.
- **"Interrupts"** = the `SIGALRM` signal. Critical sections block/unblock
  it with `sigprocmask` on `interrupts::clock_set`. `signal_nesting` (global,
  `extern "C"`) tracks handler mode.
- **SysTick**: `setitimer(ITIMER_REAL)` at `OS_INTEGER_SYSTICK_FREQUENCY_HZ`
  (default 1000); the handler calls `os_systick_handler()`.
- **High-res clock**: `gettimeofday()`, fixed 1 MHz.
- **Stacks**: 64-bit elements; min 32 KiB, default doubled on Linux.
- **Startup**: execution starts in normal `main()`; static constructor order
  is unspecified, so registries must rely on BSS zero-init and not clear
  themselves in constructors.
- **Trace** (only when `TRACE` is defined): output selected by
  `OS_USE_TRACE_POSIX_STDOUT` (default used by tests), `..._STDERR`, or the
  buffered `..._FWRITE_STDOUT/STDERR` (not thread-safe, may hang). Works
  before any init, from the first static constructor.

Platform-specific code is guarded by `__APPLE__` / `__linux__`; both sources
compile to nothing on other systems. `struct sigaction` field access differs
between macOS and Linux — keep both branches.

## Build and test

There is nothing to build in this repo on its own. It is built and tested
from the core repo's test harness:
`../micro-os-plus-iii.git/tests` (`platforms/native/`), which links this
package via xpm (`xpm run link-deps --config native-cmake-*`), e.g.:

```sh
cd ../micro-os-plus-iii.git/tests
xpm run test-native-cmake-clang   # macOS / Linux
xpm run test-native-cmake-gcc     # Linux
```

Native tests build with `-Werror` and `_POSIX_C_SOURCE=200809L`; consumers
must also define `_XOPEN_SOURCE=700L` (or `600L`). Code must be warning-free
under GCC and clang, which is why diagnostics are suppressed locally with
`#pragma GCC diagnostic push/pop` around specific lines — follow that pattern
rather than global flags.

## Conventions

- C++20, formatted with the repo `.clang-format` (GNU style: space before
  `(`, return type on its own line, indented namespaces, `Left` pointers,
  includes not sorted). Closing namespaces carry `/* namespace x */` comments.
- MIT header block (`Copyright (c) <first>-<current> Liviu Ionescu`) on every
  source file.
- Debug tracing hooks use `OS_TRACE_RTOS_THREAD_CONTEXT`,
  `OS_TRACE_RTOS_SCHEDULER`, `OS_TRACE_RTOS_SYSCLOCK_TICK`.

## Branches and releases

- Work on `xpack-development`; `xpack` is the stable branch (merged on
  release). `master` is unused.
- Release flow is in `README-MAINTAINER.md`: bump `package.json`, update
  `README*.md` (install snippet carries the version tag) and `CHANGELOG.md`
  (entries are dated `## YYYY-MM-DD` with `* <hash> <subject>` lines),
  commit "prepare vX.Y.Z", then `npm version X.Y.Z` (postversion pushes
  branches and tags).
- `.npmignore` excludes `test*/`, `scripts`, `README-*`, `NOTES.md`; check
  `npm pack` output when adding files.
- `NOTES.md` has developer notes (limitations, debugging with CodeLLDB).
