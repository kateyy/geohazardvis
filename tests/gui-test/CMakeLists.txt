
set(target gui-test)
message(STATUS "Test ${target}")


set(sources
    ${TEST_MAIN_CPP_FILEPATH}
    ColorMapping_test.cpp
    DataMapping_test.cpp
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
    PUBLIC
        gui
        ${GTEST_LIBRARIES}

    PRIVATE
        TestUtilities
)

configure_cxx_target(${target} NO_CPPCHECK)
setupProjectUserConfig(${target} GTEST_TARGET)
