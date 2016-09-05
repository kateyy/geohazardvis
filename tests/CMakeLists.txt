
add_subdirectory(TestEnvironment)

set(TestEnvIncludeDir ${CMAKE_CURRENT_SOURCE_DIR}/TestEnvironment)


# This policy was introduced in 3.0.0 and does not allow targets named "test" or "help" ...
# Since we do not rely on cmake testing we stick to the old policy for now.
if(POLICY CMP0037)
    cmake_policy(SET CMP0037 OLD)
endif()

# Function: Build test and add command to execute it via target 'test'
function(add_test_without_ctest target)

    add_subdirectory(${target})
    add_dependencies(test ${target})
    if(TARGET gtest)
        add_dependencies(${target} gtest)
    endif()
    add_custom_command(TARGET test POST_BUILD
        COMMAND $<TARGET_FILE:${target}> --gtest_output=xml:gtests.xml)

endfunction()


if(OPTION_BUILD_TESTS AND GTEST_FOUND)

    include_directories(
        SYSTEM ${GTEST_INCLUDE_DIR}
    )

    # Target 'test'
    add_custom_target(test)
    set_target_properties(test PROPERTIES EXCLUDE_FROM_DEFAULT_BUILD 1)

    # Tests
    add_test_without_ctest(core-test)
    add_test_without_ctest(gui-test)

endif()
