[OGDF](../README.md) » Compiler Defines

# Compiler Defines {#defines}

Here we list some of the available compiler definitions and macros.

## Defines set by OGDF

| **Definition**                | **Location**       | **Description**  |
|-------------------------------|--------------------|------------------|
| `OGDF_DEBUG`          | build configuration | Perform OGDF assertions if compiled in Debug mode.
| `OGDF_HEAVY_DEBUG`    | build configuration | Perform demanding OGDF assertions that might heavily increase runtime.
| `OGDF_USE_ASSERT_EXCEPTIONS` | build configuration | Throw exceptions instead of exiting by `assert()`.
| `OGDF_USE_ASSERT_EXCEPTIONS_WITH_STACK_TRACE` | build configuration | If assert exceptions are thrown, add a stack trace in the `what()` string.
| `OGDF_FUNCTION_NAME`        | build configuration        | Either `__func__` or `__PRETTY_FUNCTION__` if available. Only set if `OGDF_DEBUG` and `OGDF_USE_ASSERT_EXCEPTIONS` is set.
| `OGDF_DLL`                  | build configuration        | Building or using ODFG as a DLL.
| `OGDF_INSTALL`              | build configuration        | Building ODFG as a DLL.
| `OGDF_MEMORY_POOL_TS`       | build configuration        | OGDF uses the custom thread-safe pool memory manager (default).
| `OGDF_MEMORY_POOL_NTS`      | build configuration        | OGDF uses the custom non-thread-safe pool memory manager.
| `OGDF_MEMORY_MALLOC_TS`     | build configuration        | OGDF uses the default c++ memory manager.
| `OGDF_HAS_LINUX_CPU_MACROS` | build configuration        | Set if macros like `CPU_SET` are available.
| `OGDF_SSE3_EXTENSIONS`      | build configuration        | Set to the (system-specific) path of intrinsics (eg, `<intrin.h>`) or not defined.
| `COIN_OSI_CLP`              | build configuration        | Clp is the linear program solver (default).
| `COIN_OSI_GRB`              | build configuration        | Gurobi is the linear program solver.
| `COIN_OSI_CPX`              | build configuration        | CPLEX is the linear program solver.
| `OGDF_SYSTEM_WINDOWS`       | `basic/internal/config.h`  | Compiling for a Windows system.
| `OGDF_SYSTEM_UNIX`          | `basic/internal/config.h`  | Compiling for a UNIX system.
| `OGDF_SYSTEM_OSX`           | `basic/internal/config.h`  | Compiling for a Mac OS X system; in this case `OGDF_SYSTEM_UNIX` is also defined.
| `OGDF_ARCH_X86`       | `basic/System.h` | Compiling for a 32-bit x86 (Intel/AMD) architecture.
| `OGDF_ARCH_X64`       | `basic/System.h` | Compiling for a 64-bit x64 (Intel/AMD) architecture.

# Useful Macros

You can find a list of useful macros in the Doxygen documentation.
