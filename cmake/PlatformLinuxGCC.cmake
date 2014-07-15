
# support for C++11 etc.

execute_process(COMMAND ${CMAKE_C_COMPILER} -dumpversion
	OUTPUT_VARIABLE GCC_VERSION)

if(NOT GCC_VERSION VERSION_EQUAL 4.9)
    message(WARNING "Warning: only tested with gcc 4.9, but ${GCC_VERSION} was found.")
endif()

LIST(APPEND DEFAULT_COMPILE_DEFS

    LINUX                     # Linux system
    PIC                       # Position-independent code
    _REENTRANT                # Reentrant code
)

set(LINUX_COMPILE_FLAGS 
    -std=gnu++11 -pthread -pipe -fPIC -Wreturn-type -Wall -Wextra -Wcast-align -Wno-unused-parameter -Wno-unused-variable -Wno-unused-but-set-variable)
# pthread       -> use pthread library
# no-rtti       -> disable c++ rtti
# no-exceptions -> disable exception handling
# pipe          -> use pipes
# fPIC          -> use position independent code
# -Wreturn-type -Werror=return-type -> missing returns in functions and methods are handled as errors which stops the compilation
# -Wshadow -> e.g. when a parameter is named like a member, too many warnings, disabled for now

set(DEFAULT_COMPILE_FLAGS ${LINUX_COMPILE_FLAGS})


set(LINUX_LINKER_FLAGS "-pthread")

set(DEFAULT_COMPILE_FLAGS ${LINUX_COMPILE_FLAGS})

set(DEFAULT_LINKER_FLAGS_RELEASE ${LINUX_LINKER_FLAGS})
set(DEFAULT_LINKER_FLAGS_DEBUG ${LINUX_LINKER_FLAGS})
set(DEFAULT_LINKER_FLAGS ${LINUX_LINKER_FLAGS})

# Add platform specific libraries for linking
set(EXTRA_LIBS "")
