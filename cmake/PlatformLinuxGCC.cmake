message(STATUS "Configuring for platform Linux/GCC.")


# Enable C++11 support

execute_process(COMMAND ${CMAKE_C_COMPILER} -dumpversion
    OUTPUT_VARIABLE GCC_VERSION)

if(NOT GCC_VERSION VERSION_GREATER 4.9)
    message(WARNING "Warning: only tested with gcc 4.9, but ${GCC_VERSION} was found.")
endif()


set(LINUX_COMPILE_DEFS
    LINUX                     # Linux system
    PIC                       # Position-independent code
    _REENTRANT                # Reentrant code
)
set(DEFAULT_COMPILE_DEFS_DEBUG
    ${LINUX_COMPILE_DEFS}
    _DEBUG                    # Debug build
)
set(DEFAULT_COMPILE_DEFS_RELEASE
    ${LINUX_COMPILE_DEFS}
    NDEBUG                    # Release build
)

if (OPTION_ERRORS_AS_EXCEPTION)
    set(EXCEPTION_FLAG "-fexceptions")
else()
    set(EXCEPTION_FLAG "-fno-exceptions")
endif()

set(LINUX_COMPILE_FLAGS
      -std=c++1y

      ${EXCEPTION_FLAG}
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

      -Wno-missing-field-initializers

      -Werror=return-type # -> missing returns in functions and methods are handled as errors which stops the compilation

)

set(DEFAULT_COMPILE_FLAGS
    ${LINUX_COMPILE_FLAGS}
    $<$<CONFIG:Debug>:
    >
    $<$<CONFIG:Release>:
    >
)

set(LINUX_LINKER_FLAGS "-pthread")

set(DEFAULT_LINKER_FLAGS_RELEASE ${LINUX_LINKER_FLAGS})
set(DEFAULT_LINKER_FLAGS_DEBUG ${LINUX_LINKER_FLAGS})
set(DEFAULT_LINKER_FLAGS ${LINUX_LINKER_FLAGS})

# Add platform specific libraries for linking
set(EXTRA_LIBS "")
