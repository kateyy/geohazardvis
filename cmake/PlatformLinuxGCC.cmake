message(STATUS "Configuring for platform Linux/GCC.")


execute_process(COMMAND ${CMAKE_C_COMPILER} -dumpversion
    OUTPUT_VARIABLE GCC_VERSION)

if(NOT GCC_VERSION VERSION_GREATER 4.9)
    message(WARNING "GCC version 4.9 is required, but ${GCC_VERSION} was found. Build will probably fail.")
endif()


list(APPEND DEFAULT_COMPILE_DEFS
    LINUX                     # Linux system
    PIC                       # Position-independent code
    _REENTRANT                # Reentrant code
)

set(DEFAULT_COMPILE_FLAGS
      -fexceptions

      # TODO set these flags using CMake properties, once minimum CMake version is 3.0
      -fvisibility=hidden
      -fvisibility-inlines-hidden

      -pthread      # -> use pthread library
    # -no-rtti      # -> disable c++ rtti
      -pipe         # -> use pipes
      -Wall         # ->
      -Wextra       # ->
    # -Werror       # ->
      -fPIC         # -> use position independent code

    # -Wreturn-type
    # -Wfloat-equal
    # -Wshadow
      -Wcast-align
      -Wconversion

      -Werror=return-type # -> missing returns in functions and methods are handled as errors which stops the compilation
)

if (CMAKE_VERSION VERSION_LESS 3.1)
    list(APPEND DEFAULT_COMPILE_FLAGS -std=c++1y)
endif()

set(DEFAULT_LINKER_FLAGS "-pthread")

set(DEFAULT_LINKER_FLAGS_RELEASE ${DEFAULT_LINKER_FLAGS})
set(DEFAULT_LINKER_FLAGS_DEBUG ${DEFAULT_LINKER_FLAGS})
