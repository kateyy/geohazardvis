message(STATUS "Configuring for platform Linux/Clang.")


list(APPEND DEFAULT_COMPILE_DEFS
    LINUX                     # Linux system
    PIC                       # Position-independent code
    _REENTRANT                # Reentrant code

    $<$<CONFIG:Debug>:      -D_FORTIFY_SOURCE=2>
)

set(DEFAULT_COMPILE_FLAGS
      -std=c++1y

      -fexceptions
      -pthread      # -> use pthread library
      -pipe         # -> use pipes
      -Wall         # ->
      -Wextra       # ->
      -fPIC         # -> use position independent code

      -fsanitize=safe-stack

      -Wcast-align
      -Wconversion

      -Werror=return-type # -> missing returns in functions and methods are handled as errors which stops the compilation

      $<$<CONFIG:Debug>:
            -fstack-protector
      >

)

set(DEFAULT_LINKER_FLAGS "-pthread -fPIE -pie -fuse-ld=gold")

set(DEFAULT_LINKER_FLAGS_RELEASE "${DEFAULT_LINKER_FLAGS} -flto -Wl,-z,now -Wl,-z,relro")
set(DEFAULT_LINKER_FLAGS_DEBUG ${DEFAULT_LINKER_FLAGS})
