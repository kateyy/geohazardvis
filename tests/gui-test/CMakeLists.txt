
set(target gui-test)
message(STATUS "Test ${target}")


include_directories(
    BEFORE
    ${CMAKE_SOURCE_DIR}
)

set(libs
    gui
    ${GTEST_LIBRARIES}
)


set(sources
    main.cpp
    RenderView_test.cpp
    RenderView_test_tools.h
    RenderView_test_tools.cpp
)


# Build executable

add_executable(${target} ${sources})

target_link_libraries(${target} ${libs})

configure_cxx_target( ${target} )
setupProjectUserConfig(${target})
