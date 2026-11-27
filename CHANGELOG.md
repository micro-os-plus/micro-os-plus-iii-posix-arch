# Change & release log

Releases in reverse chronological order.

Please check
[GitHub](https://github.com/micro-os-plus/posix-arch/issues)
and close existing issues and pull requests.

## 2026-11-27

* v1.1.0
* 48655ea os-core.cpp: silence the clang 20 warning

## 2026-11-25

* 682e383 os-inlines.h: add the empty set_stack_limit()

## 2026-11-24

* c9c081c CODE-REVIEW-1.md: code review of the POSIX port
* ee557c8 README.md, CLAUDE.md, os-core.cpp: document the lost ticks
* e21e63d os-inlines.h: wait for interrupts with sigsuspend() (#6)
* fb05566 os-core.cpp: hrclock from CLOCK_MONOTONIC (#5)
* d677edb os-core.cpp: new threads start with the clock signal enabled (#4)

## 2026-11-23

* 3802953 os-core.cpp: instrumentation event for the first thread
* 3788112 README.md, CLAUDE.md, port headers: update the documentation
* 17db682 os-c-decls.h: require _XOPEN_SOURCE only for the system ucontext
* b8be14d os-inlines.h, os-core.cpp: document signal_nesting
* df0207f os-core.cpp: setitimer() period in seconds and microseconds (#3)
* 1b4b204 .npmignore: exclude CLAUDE.md, .claude/ and maintenance/
* 9220598 os-core.cpp: no reschedule before the scheduler is started
* 32ed2fa os-core.cpp: record a reschedule requested while locked
* 5ef0ad5 rename tests
* 93394dd add maintenance

## 2026-10-01

* 49282e6 add docs/preemption plan
* 44f3ca7 add copilot & claude files

## 2025-08-24

* 86a6a1f 1.0.1
* 405bde4 rename xpack-development
* 937c920 prepare v1.0.1
* 879d82e .gitignore remove test*
* ed051de add .clang-format & re-format
* c6758e7 update copyright notices

## 2023-07-22

* 4523e79 README updates

## 2023-07-21

* fd434d0 README updates
* 0a6c645 README updates

## 2023-07-18

* f52c3f4 README update

## 2023-07-14

* 97434db README update
* d964f19 README update
* a5dd774 README updates
* 20baca5 .npmignore scripts
* 9068a7b 1.0.0
* a99c394 .npmignore update
* 2678362 prepare v1.0.0
* 3f1abef cleanups, remove Eclipse files

## 2023-06-30

* e802595 package.json: 1.0.0-pre.11
* f0331d6 cosmetics

## 2023-06-26

* 12a323d package.json: 1.0.0-pre.10
* 56f8aa6 use #if defined(OS_USE_OS_APP_CONFIG_H)

## 2023-06-23

* e651bb5 package.json: 1.0.0-pre.9
* 3d7abe6 os-core.cpp: OS_TRACE_RTOS_SYSCLOCK_TICK

## 2023-06-22

* 070afed update copyright notice
* 823c0bd trace-posix: add warnings
* e3deeec package.json: 1.0.0-pre.8
* f046f7c os_impl_ucontext

## 2023-06-21

* ce53775 CMakeLists.txt: fix how to use comments

## 2023-06-17

* 0c71718 README update

## 2023-06-16

* e2c5abd package.json: 1.0.0-pre.7
* 4b946ff os-core.cpp: use libucontext_posix_*

## 2023-06-15

* ba47a7d package.json: 1.0.0-pre.6
* c95bf17 #2: add trace streams via buffered files

## 2023-06-13

* 90dd87d 1.0.0-pre.5
* fc77d50 os-decls.h: SIGSTKSZ -> 32KB
* 05416aa package.json: 1.0.0-pre.4
* 2a0cbd5 #1: update references to libucontext_*

## 2023-06-09

* a34172d NOTES.md update
* 7494f3e package.json 1.0.0-pre.3
* 7ff071e move inlines to os-core.cpp, {c c}, {u u}

## 2023-06-08

* 628d019 package.json: 1.0.0-pre.2
* 794232d trace-posix.cpp: try sync(), but slow and faulty
* 217be4d remove empty statements
* f535cbd comment out struct & class
* b972d59 add #pragma to silence warnings
* 1db72ae os-decls.h: unsigned clock::signal_number

## 2023-06-06

* aac9d86 README: update known problems
* b66f543 rename micro-os-plus-iii-posix-arch
* a5e196c add .vscode/settings.json
* 7c4127f add CMakeLists.txt with micro-os-plus::posix-arch
* fb5c072 silence clang warnings

## 2023-06-05

* 8aba4f7 add package.json and make the project an xPack
