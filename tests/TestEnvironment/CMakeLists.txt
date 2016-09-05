
set(target TestEnvironment)


set(sources
    TestEnvironment.h
    TestEnvironment.cpp
)

add_library(${target} STATIC ${sources})

target_link_libraries(${target}
    Qt5::Widgets
)

configure_cxx_target(${target})
