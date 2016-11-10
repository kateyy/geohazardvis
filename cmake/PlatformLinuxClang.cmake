message(STATUS "Configuring for platform Linux/Clang.")


list(APPEND DEFAULT_COMPILE_DEFS
    LINUX                     # Linux system
    PIC                       # Position-independent code
    _REENTRANT                # Reentrant code

    $<$<CONFIG:Debug>:      -D_FORTIFY_SOURCE=2>
)

set(DEFAULT_COMPILE_FLAGS
    -fexceptions
    # use position independent code
    # http://blog.quarkslab.com/clang-hardening-cheat-sheet.html
    # (-pie dropped in version 3.8?)
    $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>: -fPIE
        $<$<VERSION_LESS:$<CXX_COMPILER_VERSION>,3.8>: -pie>>
    $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>: -fPIC>
    -fsanitize=safe-stack
    # http://clang.llvm.org/docs/ControlFlowIntegrity.html
    # LTO (required for CFI is currently broken in Ubuntu packaged clang)
#     -fsanitize=cfi
#     -flto

    -Wall
    -Wextra
    -Wunused

    -Wreorder
    -Wignored-qualifiers
    -Wreturn-type
    -Wswitch
    -Wswitch-default
    -Wuninitialized
    -Wmissing-field-initializers
    -Wpedantic
    -Wreturn-stack-address
    $<$<NOT:$<VERSION_LESS:CXX_COMPILER_VERSION,3.9>>: -Wcomma>

    -Werror=format-security

    $<$<CONFIG:Debug>:
        -fstack-protector
    >

    # colors in output of ninja builds
    -fcolor-diagnostics

    -Wno-missing-braces

    # Disable warnings caused by VTK headers
    -Wno-extra-semi
)

if (CMAKE_VERSION VERSION_LESS 3.1)
    list(APPEND DEFAULT_COMPILE_FLAGS -std=c++14)
endif()

if (NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 3.8)  # VERSION_GREATER_EQUAL available in CMake 3.7
    set(OPTION_CLANG_DEBUGGER gdb CACHE STRING "Debugger for that clang optimizes the DWARF debugging information")
    set_property(CACHE OPTION_CLANG_DEBUGGER PROPERTY STRINGS gdb lldb)
    list(APPEND DEFAULT_COMPILE_FLAGS $<$<CONFIG:Debug>: -g${OPTION_CLANG_DEBUGGER}>)
endif()

set(DEFAULT_LINKER_FLAGS
    -fsanitize=safe-stack
    -Wl,-z,relro    # read-only relocations...
    -Wl,-z,now      # ... and immediate binding (symbols resolved at load time)
)
# set(DEFAULT_LINKER_FLAGS ${DEFAULT_LINKER_FLAGS} -flto) # currently broken, at least on Ubuntu
