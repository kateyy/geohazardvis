
set(target core-test)
message(STATUS "Test ${target}")


include_directories(
    BEFORE
    ${CMAKE_SOURCE_DIR}
)

set(libs
    core
    ${GTEST_LIBRARIES}
)


set(sources
    main.cpp
    DataSetHandler_test.cpp
)


# Build executable

add_executable(${target} ${sources})

target_link_libraries(${target} ${libs})

configure_cxx_target( ${target} )
setupProjectUserConfig(${target})
