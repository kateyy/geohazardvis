
set(target gui-test)
message(STATUS "Test ${target}")


set(sources
    main.cpp
    app_helper.h
    DEMWidget_test.cpp
    RenderView_test.cpp
    RenderView_test_tools.h
    RenderView_test_tools.cpp
    RenderViewStrategy_test.cpp
)

add_executable(${target} ${sources})

target_link_libraries(${target} 
    gui
    ${GTEST_LIBRARIES}
)

configure_cxx_target(${target})
setupProjectUserConfig(${target})
