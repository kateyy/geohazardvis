
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
    BinaryFile_test.cpp
    DataObject_test.cpp
    DataSetHandler_test.cpp
    RenderedData_test.cpp
    VTKAlgorithmImplementations_test.cpp
)


# Build executable

add_executable(${target} ${sources})

target_link_libraries(${target} ${libs})

configure_cxx_target( ${target} )
setupProjectUserConfig(${target})
