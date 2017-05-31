message(STATUS "Configuring for platform Linux/GCC.")


execute_process(COMMAND ${CMAKE_CXX_COMPILER} -dumpversion
    OUTPUT_VARIABLE GCC_VERSION)

if(NOT GCC_VERSION VERSION_GREATER 4.9)
    message(WARNING "GCC version 4.9 is required, but ${GCC_VERSION} was found. Build will probably fail.")
endif()


list(APPEND DEFAULT_COMPILE_DEFS
    LINUX                     # Linux system
    _GLIBCXX_DEBUG
)

set(DEFAULT_COMPILE_FLAGS
      -fexceptions

      -pthread      # -> use pthread library
    # -no-rtti      # -> disable c++ rtti
      -Wall         # ->
      -Wextra       # ->
    # -Werror       # ->

    # -Wreturn-type
    # -Wfloat-equal
    # -Wshadow
      -Wcast-align
      -Wconversion

      -Werror=return-type # -> missing returns in functions and methods are handled as errors which stops the compilation
)

set(DEFAULT_LINKER_FLAGS "-pthread")

set(DEFAULT_LINKER_FLAGS_RELEASE ${DEFAULT_LINKER_FLAGS})
set(DEFAULT_LINKER_FLAGS_DEBUG ${DEFAULT_LINKER_FLAGS})
