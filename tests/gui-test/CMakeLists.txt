
set(target gui-test)
message(STATUS "Test ${target}")


set(sources
    ${TEST_MAIN_CPP_FILEPATH}
    DataMapping_test.cpp
    data_view/RenderViewStrategy2D_test.cpp
    data_view/RenderView_test.cpp
    data_view/RenderView_test_tools.cpp
    data_view/RenderView_test_tools.h
    data_view/RendererImplementation3D_test.cpp
    data_view/RendererImplementationBase3D_test.cpp
    data_view/ResidualView_test.cpp
    rendering_interaction/Picker_test.cpp
    widgets/DEMWidget_test.cpp
)

source_group_by_path_and_type(${CMAKE_CURRENT_SOURCE_DIR} ${sources})
source_group_by_path(${CMAKE_CURRENT_SOURCE_DIR} ".*" "" "CMakeLists.txt")

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
