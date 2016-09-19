
set(target gui-test)
message(STATUS "Test ${target}")


set(sources
    main.cpp
    ColorMapping_test.cpp
    DEMWidget_test.cpp
    Picker_test.cpp
    RendererImplementationBase3D_test.cpp
    RenderView_test.cpp
    RenderView_test_tools.h
    RenderView_test_tools.cpp
    RenderViewStrategy_test.cpp
    ResidualView_test.cpp
)

add_executable(${target} ${sources})

target_link_libraries(${target}
    gui
    ${GTEST_LIBRARIES}
    TestEnvironment
)

target_include_directories(${target}
    PRIVATE ${TestEnvIncludeDir}
)

configure_cxx_target(${target} NO_CPPCHECK)
setupProjectUserConfig(${target})
