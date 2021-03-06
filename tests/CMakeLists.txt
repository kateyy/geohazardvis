
add_subdirectory(TestUtilities)


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
        COMMAND $<TARGET_FILE:${target}> --gtest_output=xml:gtests.xml
        USES_TERMINAL)

endfunction()


if(OPTION_BUILD_TESTS AND GTEST_FOUND)
    # Target 'test'
    add_custom_target(test)
    set_target_properties(test PROPERTIES EXCLUDE_FROM_DEFAULT_BUILD 1)

    set(TEST_MAIN_CPP_FILEPATH "${CMAKE_CURRENT_SOURCE_DIR}/test-main.cpp")

    # This is for an incompatibility between Google Test 1.8.0 and MSVC 15 2017 version 15.5
    # See https://github.com/google/googletest/issues/1318
    if (MSVC AND MSVC_VERSION VERSION_GREATER 1411)
        add_definitions(/D_SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING)
    endif()

    # Tests
    add_test_without_ctest(core-test)
    add_test_without_ctest(gui-test)
endif()
