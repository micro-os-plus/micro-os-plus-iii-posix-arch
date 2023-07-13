# Change & release log

Releases in reverse chronological order.

Please check
[GitHub](https://github.com/micro-os-plus/posix-arch/issues)
and close existing issues and pull requests.

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
