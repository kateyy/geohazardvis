
set(target TestUtilities)


set(sources
    TestEnvironment.h
    TestEnvironment.cpp
)

add_library(${target} STATIC ${sources})

target_include_directories(${target}
    INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${target}
    PUBLIC
        Qt5::Widgets
)

configure_cxx_target(${target})
