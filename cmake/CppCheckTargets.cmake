
# https://blog.kitware.com/how-many-ya-got/
if(NOT DEFINED PROCESSOR_COUNT)

    set(PROCESSOR_COUNT 0)

    # Linux:
    set(cpuinfo_file "/proc/cpuinfo")
    if(EXISTS "${cpuinfo_file}")
        file(STRINGS "${cpuinfo_file}" procs REGEX "^processor.: [0-9]+$")
        list(LENGTH procs PROCESSOR_COUNT)
    endif()

    # Mac:
    if(APPLE)
        find_program(cmd_sys_pro "system_profiler")
        if(cmd_sys_pro)
            execute_process(COMMAND ${cmd_sys_pro} OUTPUT_VARIABLE info)
            string(REGEX REPLACE "^.*Total Number Of Cores: ([0-9]+).*$" "\\1"
            PROCESSOR_COUNT "${info}")
        endif()
    endif()

    # Windows:
    if(WIN32)
        set(PROCESSOR_COUNT "$ENV{NUMBER_OF_PROCESSORS}")
    endif()

endif()


if (OPTION_ADD_CPPCHECK_TARGETS AND (NOT CPPCHECK_EXECUTABLE OR NOT EXISTS ${CPPCHECK_EXECUTABLE}))
    find_program(CPPCHECK_EXECUTABLE cppcheck
        PATH_SUFFIXES cppcheck
    )
endif()

set(RUN_CPPCHECK OFF)

if (OPTION_ADD_CPPCHECK_TARGETS AND (NOT CPPCHECK_EXECUTABLE OR NOT EXISTS ${CPPCHECK_EXECUTABLE}))
    message("Could not find cppcheck. Skipping cppcheck targets.")
else()
    # "cppcheck --version" -> "Cppcheck x.y.z"
    execute_process(COMMAND ${CPPCHECK_EXECUTABLE} --version
        RESULT_VARIABLE cppcheckVersionCheckResult
        OUTPUT_VARIABLE cppcheckVersionCheckOutput
        ERROR_VARIABLE cppcheckVersionCheckError
    )
    if (cppcheckVersionCheckResult)
        message(WARNING "Error while executing cppcheck (${cppcheckVersionCheckError})")
    else()
        string(SUBSTRING "${cppcheckVersionCheckOutput}" 8 -1 CPPCHECK_VERSION)
        string(STRIP "${CPPCHECK_VERSION}" CPPCHECK_VERSION)
        string(REPLACE "." ";" versionList ${CPPCHECK_VERSION})
        list(LENGTH versionList numVersionNumbers)
        list(GET versionList 0 CPPCHECK_VERSION_MAJOR)
        set (CPPCHECK_VERSION_MINOR 0)
        set (CPPCHECK_VERSION_PATCH 0)
        if (numVersionNumbers GREATER 1)
            list(GET versionList 1 CPPCHECK_VERSION_MINOR)
            if (numVersionNumbers GREATER 2)
                list(GET versionList 2 CPPCHECK_VERSION_PATCH)
            endif()
        endif()

        set(RUN_CPPCHECK ON)

    endif()
endif()


set(cppcheckSuppressionsFile_in ${PROJECT_SOURCE_DIR}/cmake/cppcheckSuppressions.in)
set(cppcheckSuppressionsFile ${CMAKE_BINARY_DIR}/cppcheckSuppressions.txt)

# https://arcanis.me/en/2015/10/17/cppcheck-and-clang-format/
set(cppcheckParams
    # --check-config
    --enable=warning,style,performance,portability,information,missingInclude
    --suppressions-list=${cppcheckSuppressionsFile}
    -v
    --quiet
    --template="{file} \({line}\): {message} [{severity}:{id}]"
    --library=std
    --library=qt
    --language=c++
    -j ${PROCESSOR_COUNT}
    -UQT_NAMESPACE
)
if (WIN32)
    list(APPEND cppcheckParams --library=windows)
endif()


set(cppcheckAllIncludes CACHE INTERNAL "" FORCE)
set(cppcheckAllSources CACHE INTERNAL "" FORCE)


function(cppcheck_target target)

    if (NOT RUN_CPPCHECK)
        return()
    endif()

    get_target_property(_rawSources ${target} SOURCES)
    get_target_property(_sourceDir ${target} SOURCE_DIR)
    get_target_property(_rawIncludes ${target} INCLUDE_DIRECTORIES)

    set(_sources)
    foreach(_src ${_rawSources})
        get_filename_component(absoluteFilePath ${_src} ABSOLUTE ${_sourceDir})
        list(APPEND _sources ${absoluteFilePath})
    endforeach()

    set(_includes)
    foreach(_inc ${_rawIncludes})
        if (_inc MATCHES "^${PROJECT_SOURCE_DIR}.*$")
            list(APPEND _includes "-I;${_inc}")
        endif()
    endforeach()

    set(cppcheckAllIncludes ${cppcheckAllIncludes} ${_includes} CACHE INTERNAL "" FORCE)
    set(cppcheckAllSources ${cppcheckAllSources} ${_sources} CACHE INTERNAL "" FORCE)

    add_custom_target( cppcheck_${target}
        COMMAND ${CPPCHECK_EXECUTABLE}
            ${cppcheckParams}
            ${_includes}
            ${_sources}
        SOURCES
            ${cppcheckSuppressionsFile_in}
    )
    source_group("" FILES ${cppcheckSuppressionsFile_in})
    set_target_properties( cppcheck_${target}
        PROPERTIES
        FOLDER              ${IDE_CPPCHECK_FOLDER}
        EXCLUDE_FROM_ALL    ON
    )

endfunction()


function(generate_cppcheck_suppressions)
    if (OPTION_ADD_CPPCHECK_TARGETS)
        configure_file(${cppcheckSuppressionsFile_in} ${cppcheckSuppressionsFile})
    endif()
endfunction()


function(create_cppcheck_ALL_target)

    if (NOT RUN_CPPCHECK)
        return()
    endif()

    set(target cppcheck_ALL)
    
    list(REMOVE_DUPLICATES cppcheckAllIncludes)
    list(REMOVE_DUPLICATES cppcheckAllSources)

    add_custom_target(cppcheck_ALL
        COMMAND ${CPPCHECK_EXECUTABLE}
            ${cppcheckParams}
            ${cppcheckAllIncludes}
            ${cppcheckAllSources}
        SOURCES
            ${cppcheckSuppressionsFile_in}
    )
    source_group("" FILES ${cppcheckSuppressionsFile_in})
    set_target_properties(cppcheck_ALL
        PROPERTIES
        FOLDER              ${IDE_CPPCHECK_FOLDER}
        EXCLUDE_FROM_ALL    ON
    )

endfunction()
