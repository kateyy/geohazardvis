
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

target_include_directories(${target} SYSTEM
    PUBLIC
        ${GTEST_INCLUDE_DIR}
)

target_link_libraries(${target}
    PUBLIC
        gui
        ${GTEST_LIBRARIES}

    PRIVATE
        TestUtilities
)

target_compile_definitions(${target}
    PRIVATE
        TEST_WITH_OPENGL_SUPPORT
)

configure_cxx_target(${target} NO_CPPCHECK)
setupProjectUserConfig(${target} GTEST_TARGET)
