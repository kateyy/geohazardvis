

function(get_git_commit_date hash _var)

    if(NOT GIT_FOUND)
        find_package(Git QUIET)
    endif()
    if(NOT GIT_FOUND)
        set(${_var} "" PARENT_SCOPE)
        return()
    endif()

    if(ARGC EQUAL 3)
        set(GIT_PARENT_DIR ${ARGV2})
    else()
        set(GIT_PARENT_DIR "${CMAKE_SOURCE_DIR}")
    endif()

    execute_process(COMMAND "${GIT_EXECUTABLE}" log ${hash} -n1 --format=%ad --date=iso
        WORKING_DIRECTORY "${GIT_PARENT_DIR}"
        RESULT_VARIABLE res
        OUTPUT_VARIABLE out
        ERROR_VARIABLE err
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(NOT res EQUAL 0)
        set(${_var} "" PARENT_SCOPE)
        return()
    endif()

    set(${_var} ${out} PARENT_SCOPE)

endfunction()
