
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


CMAKE_DEPENDENT_OPTION(OPTION_USE_SYSTEM_GTEST "Search for installed Google Test Libraries instead of compiling them" OFF
    "OPTION_BUILD_TESTS" OFF)

if(OPTION_BUILD_TESTS)

    if (OPTION_USE_SYSTEM_GTEST)
        find_package(GTEST)
    else()
        set(GTEST_FOUND 1)
        set(GTEST_DIR "${THIRD_PARTY_DIR}/gtest")

        ExternalProject_Add(gtest
            PREFIX ${GTEST_DIR}
            GIT_REPOSITORY "https://github.com/google/googletest"
            GIT_TAG "release-1.7.0"
            CMAKE_CACHE_ARGS
                -Dgtest_force_shared_crt:bool=ON
            INSTALL_COMMAND ""  # skip install for gtest: target does not exist
        )

        if(WIN32)
            set(GTEST_LIBRARIES "${GTEST_DIR}/src/gtest-build/$<CONFIG>/gtest.lib")
        else()
            set(GTEST_LIBRARIES "${GTEST_DIR}/src/gtest-build/libgtest.a")
        endif()

        set(GTEST_INCLUDE_DIR "${GTEST_DIR}/src/gtest/include")
    endif()

    if(NOT GTEST_FOUND)
        message(WARNING "Tests skipped: gtest not found")
    endif()
endif()

if(OPTION_BUILD_TESTS AND GTEST_FOUND)

    include_directories(
        ${GTEST_INCLUDE_DIR}
    )

    # Target 'test'
    add_custom_target(test)
    set_target_properties(test PROPERTIES EXCLUDE_FROM_DEFAULT_BUILD 1)

    # Tests
    add_test_without_ctest(core-test)
    add_test_without_ctest(gui-test)

endif()
