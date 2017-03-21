message(STATUS "Configuring for platform Windows/MSVC/Clang.")

set(DEFAULT_COMPILE_DEFS
    WIN32                       # Windows system
    _WINDOWS
    # UNICODE                     # Use unicode
    # _UNICODE                    # Use unicode
    # _SCL_SECURE_NO_WARNINGS     # Calling any one of the potentially unsafe methods in the Standard C++ Library
    # _CRT_SECURE_NO_DEPRECATE    # Disable CRT deprecation warnings
)

set(DEFAULT_COMPILE_DEFS_DEBUG
    ${DEFAULT_COMPILE_DEFS}
    _DEBUG                      # Debug build
)

set(DEFAULT_COMPILE_DEFS_RELEASE
    ${DEFAULT_COMPILE_DEFS}
    NDEBUG                      # Release build
)


set(DEFAULT_COMPILE_FLAGS
      -Wall
      -Wextra

      -Wreturn-type
      # -Wshadow      # -> e.g. when a parameter is named like a member, too many warnings, disabled for now
      -Werror=return-type # -> missing returns in functions and methods are handled as errors which stops the compilation

      -Wno-invalid-constexpr # required for qrgba64.h (Qt 5.6)
      # -Wno-unknown-warning-option
      -Wno-unknown-pragmas  # required for Qt (atomic)
      # -Wno-reinterpret-base-class
      # -Wno-overloaded-virtual
      -Wno-inconsistent-missing-override    # required for libzeug

      # -Wno-unused-parameter
      # -Wno-unused-variable
      # -Wno-unused-but-set-variable
      -Wno-microsoft
      # -Wno-deprecated-declarations
      # -Wno-pointer-bool-conversion
      # -Wno-missing-braces

      # -fms-extensions     # -> Enable full Microsoft Visual C++ compatibility
      # -fms-compatibility  # -> Accept some non-standard constructs supported by the Microsoft compiler
      -frtti
      -fexceptions
      -fcxx-exceptions
      -fno-omit-frame-pointer

    $<$<CONFIG:Debug>:
        # -gline-tables-only
        -g
        -O0
        -fno-inline
        /MDd
    >

    $<$<CONFIG:Release>:
        # -O3
        /MD
    >
)

set(DEFAULT_LINKER_FLAGS_DEBUG
    ""
)

set(DEFAULT_LINKER_FLAGS_RELEASE
    ""
)
