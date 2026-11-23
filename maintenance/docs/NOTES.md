# Developer notes

## Limitations

The µOS++ port running on synthetic POSIX platforms is fully functional, except
the scheduler runs in cooperative mode only; thread pre-emption might be
possible, but was considered not worth the effort.

## Context switching

Context switching is implemented with `makecontext()`, `getcontext()`,
`setcontext()`, available on GNU/Linux and macOS.

## Scheduler tick

The scheduler tick is implemented with `setitimer()`, which triggers a
periodic `ITIMER_REAL` signal.

## Startup

For portability reasons, execution starts in the `main()` function.

Static constructors are executed before `main()`, as usual, but the order
is not known, so the code must not depend on any specific order.

In particular, registry classes must use the BSS memory initialisation
to clear their data structures, and be sure the registry constructor
does not clear them again, since at that moment it is possible that
previous other classes to be registered.

## Trace

The default trace output is STDOUT (`OS_USE_TRACE_POSIX_STDOUT`), but
can be redirected to STDERR by using `OS_USE_TRACE_POSIX_STDERR`.

The trace output is available from the very moment, in the first static
constructor, since it does not need any initialisation.

## Memory allocation

The default memory allocator uses the system `malloc()`.

## Debug sessions in VS Code

Install in VS Code:

- Microsoft - C/C++
- Vadim Chugunov - CodeLLDB

### LD_LIBRARY_PATH

To get the toolchain libraries paths, use the `get-libraries-paths.sh` script:

```console
$ tests/platform-native/scripts/get-libraries-paths.sh build/native-cmake-clang-debug/xpacks/.bin/clang++
/home/ilg/.local/xPacks/@xpack-dev-tools/clang/15.0.7-2.1/.content/lib/clang/15.0.7:/home/ilg/.local/xPacks/@xpack-dev-tools/clang/15.0.7-2.1/.content/bin/../lib/aarch64-unknown-linux-gnu:/usr/lib/gcc/aarch64-linux-gnu/10:/lib/aarch64-linux-gnu:/usr/lib/aarch64-linux-gnu:/lib:/usr/lib
```

### lldb

- <https://github.com/vadimcn/codelldb/blob/v1.9.2/MANUAL.md>

Add a launcher:

Adjust the `PATH` to access the local `lldb` and add
`LD_LIBRARY_PATH` for the libraries:

```json
        {
            "type": "lldb",
            "request": "launch",
            "name": "native-cmake-clang-debug rtos-apis-test",
            "program": "${workspaceFolder}/build/native-cmake-clang-debug/platform-bin/rtos-apis-test",
            "args": [],
            "cwd": "${workspaceFolder}",
            "env": {
                "PATH": "${workspaceFolder}/build/native-cmake-clang-debug/xpacks/.bin:${env:PATH}",
                "LD_LIBRARY_PATH": "/home/ilg/.local/xPacks/@xpack-dev-tools/clang/15.0.7-2.1/.content/lib/clang/15.0.7:/home/ilg/.local/xPacks/@xpack-dev-tools/clang/15.0.7-2.1/.content/bin/../lib/aarch64-unknown-linux-gnu:/usr/lib/gcc/aarch64-linux-gnu/10:/lib/aarch64-linux-gnu:/usr/lib/aarch64-linux-gnu:/lib:/usr/lib"
            }
        }
```

```json
    "lldb.displayFormat": "hex"
```

### lldb-vscode

Experimental, could not install it.

- https://github.com/llvm/llvm-project/tree/main/lldb/tools/lldb-vscode

```sh
clang_path="${HOME}/.local/xPacks/@xpack-dev-tools/clang/15.0.7-2.1"

extension_path="${HOME}/.vscode-server/extensions/llvm-org.lldb-vscode-0.1.0"
extension_path="${HOME}/extensions/llvm-org.lldb-vscode-0.1.0"

mkdir -p "${extension_path}/bin"
curl -l https://raw.githubusercontent.com/llvm/llvm-project/main/lldb/tools/lldb-vscode/package.json  -o "${extension_path}/package.json"

cd "${extension_path}/bin"
ln -s "${clang_path}/.content/bin/lldb-vscode"  .
```
