[![GitHub package.json version](https://img.shields.io/github/package-json/v/micro-os-plus/micro-os-plus-iii-posix-arch)](https://github.com/micro-os-plus/micro-os-plus-iii-posix-arch/blob/xpack/package.json)
[![GitHub tag (latest by date)](https://img.shields.io/github/v/tag/micro-os-plus/micro-os-plus-iii-posix-arch)](https://github.com/micro-os-plus/micro-os-plus-iii-posix-arch/tags/)
[![license](https://img.shields.io/github/license/micro-os-plus/micro-os-plus-iii-posix-arch)](https://github.com/micro-os-plus/micro-os-plus-iii-posix-arch/blob/xpack/LICENSE)

# A source code library with the µOS++ POSIX synthetic port (an xpm/npm package)

This project provides support for running µOS++ on POSIX
platforms like macOS and GNU/Linux.

The open-source project is hosted on GitHub as
[micro-os-plus/micro-os-plus-iii-posix-arch](https://github.com/micro-os-plus/micro-os-plus-iii-posix-arch).

## Maintainer info

This page is addressed to developers who plan to include this source
library into their own projects.

For maintainer info, please see the
[README-MAINTAINER](maintenance/docs/README-MAINTAINER.md) file.

## Install

As a source code library, this project can be integrated into another project
in the traditional way,
by either copying the relevant files into the target project, or by linking
the entire project as a Git submodule.

However, the workflow can be further automated and the most convenient way is
to **add it as a dependency** to the project via **xpm**.

### Install with xpm/npm

Along with the source files, this project also includes a
`package.json` file with the metadata that allows it to be identified as an
**xpm/npm** package that can be installed automatically as a dependency.

#### Prerequisites

A recent [xpm](https://xpack.github.io/xpm/),
which is a portable [Node.js](https://nodejs.org/) command line application
that complements [npm](https://docs.npmjs.com)
with several extra features specific to
**C/C++ projects**.

It is recommended to install/update to the latest version with:

```sh
npm install --global xpm@latest
```

For details please follow the instructions in the
[xPack install](https://xpack.github.io/install/) page.

Warning: Be sure **xpm** is not installed with administrative/root rights.

#### xpm

This source code library can be installed as a sub-folder below `xpacks` with:

```sh
cd my-project
xpm init # Add a package.json if not already present

xpm install github:micro-os-plus/micro-os-plus-iii-posix-arch#v1.0.1 --save-dev --copy

ls -l xpacks/@micro-os-plus/micro-os-plus-iii-posix-arch
```

Note: On Windows use `dir` and back-slashes.

#### npm

The package can also be installed with [npm](https://docs.npmjs.com)
or related, but the content will end in the `node_modules` folder,
along with JavaScript code;
therefore it is recommended to install C/C++ packages via **xpm**.

### Add as a Git submodule

If, for any reason, xpm/npm are not available, it is always possible
to manually copy the relevant files into the target
project. However, this will need extra maintenance to keep the
project up to date.

A more convenient
solution is to link the entire project as a **Git submodule**,
for example below an `xpacks` folder:

```sh
cd my-project
git init # Unless already a Git project
mkdir -p xpacks

git submodule add https://github.com/micro-os-plus/micro-os-plus-iii-posix-arch.git \
  xpacks/@micro-os-plus/micro-os-plus-iii-posix-arch
```

## Branches

Apart from the unused `master` branch, there are two active branches:

- `xpack`, with the latest stable version (default)
- `xpack-development`, with the current development version

All development is done in the `xpack-development` branch, and contributions via
Pull Requests should be directed to this branch.

When new releases are published, the `xpack-development` branch is merged
into `xpack`.

## Developer info

### Overview

The µOS++ IIIe source code is split between a portable part
and platform specific code (like synthetic POSIX, this project).
Applications must include both.

### Status

The **micro-os-plus-iii-posix-arch** source library is fully functional
on macOS and GNU/Linux.

Note: The next edition of the project (IVe) is currently work in progress,
with the monolithic
repository to be split into multiple separate source code libraries.

### Build & integration info

The project is written in C/C++ and it is expected
to be used in C and C++ projects.

The source code was compiled with GCC 11-14 and clang 13-19 (on macOS
and GNU/Linux), and should be warning free.

To ease the integration of this package into user projects, there
are already made CMake configuration files (see below).

For other build systems, consider the following details:

#### Include folders

The following folders should be passed to the compiler during the build:

- `include`

#### Source files

The source files to be added to the build are:

- `src/diag/trace-posix.cpp`
- `src/rtos/os-core.cpp`

#### Preprocessor definitions

- `OS_INCLUDE_LIBUCONTEXT`, to use the context functions from
  [libucontext](https://github.com/xpack-3rd-party/libucontext-xpack)
  (recommended; this is what the tests use); usually defined in
  `<cmsis-plus/os-app-config.h>`;
- otherwise, `_XOPEN_SOURCE=700L` (or `600L`), required by the system
  `<ucontext.h>` (deprecated by POSIX; this configuration is not tested).

#### Compiler options

- `-std=c++20` or higher for C++ sources
- `-std=c11` for C sources

#### Dependencies

- the µOS++ IIIe portable part, `@micro-os-plus/micro-os-plus-iii`
  (the CMake target `micro-os-plus::iii`);
- with `OS_INCLUDE_LIBUCONTEXT`, `@xpack-3rd-party/libucontext`
  (the CMake target `xpack-3rd-party::libucontext`).

#### CMake

To integrate the **micro-os-plus-iii-posix-arch** source library
into a CMake application,
add this folder to the build:

```cmake
add_subdirectory("xpacks/@micro-os-plus/micro-os-plus-iii-posix-arch")
```

The result is an interface library that can be added as an application
dependency with:

```cmake
target_link_libraries(your-target PRIVATE

  micro-os-plus::iii-posix-arch
)
```

### Examples

- none

### Known problems

- none

### Limitations

The port runs µOS++ as a plain process, with the RTOS threads as
`ucontext` contexts and the interrupts simulated with signals:

- the scheduler is cooperative (non-preemptive): a thread made ready
  by an interrupt (for example by the system tick) runs when the
  running thread next calls the RTOS (yields, waits, or releases a
  resource);
- the system tick is the `SIGALRM` signal (`setitimer()`); the tick
  processing, the timer callbacks and any other code called from
  interrupts run inside a signal handler, so they may call only
  async-signal-safe functions (not, for example, `printf()` or
  `malloc()`; see
  [micro-os-plus-iii#139](https://github.com/micro-os-plus/micro-os-plus-iii/issues/139));
- code that simulates other interrupts with signals must increment
  `signal_nesting` around its handlers, so that the RTOS sees them in
  handler mode (as the CMSIS RTOS validator does with `SIGUSR1`);
- the signal handlers run on the stack of the interrupted thread, so
  each thread stack must also have room for a signal frame (the
  minimum stack size is 32 KiB);
- the timing is that of a host process: the system tick is the
  `SIGALRM` signal, which does not queue, so when it is delayed (by the
  host scheduling or, on macOS, by timer coalescing on an idle host)
  the delayed ticks are lost, and `sysclock`, the tick based timeouts
  and timers, and `rtclock` (derived from the tick) run slower than
  real time; about 8% slower was measured on an idle macOS host, no
  loss on GNU/Linux or on a busy macOS host; `hrclock` follows real
  time. The lost ticks are not compensated, so each signal processes
  exactly one tick, as a real tick interrupt does.

### Tests

The port is tested with the tests of the portable part
(`micro-os-plus-iii`, in the `tests` folder, the `native` platform),
with GCC and clang, on macOS and GNU/Linux.

The `tests/trace-stdout` and `tests/trace-stderr` folders contain small
trace programs, not built by the tests.

## License

Unless otherwise stated, the content is released under the terms of the
[MIT License](https://opensource.org/licenses/mit/),
with all rights reserved to
[Liviu Ionescu](https://github.com/ilg-ul).
