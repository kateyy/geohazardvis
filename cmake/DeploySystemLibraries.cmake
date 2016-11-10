
if (MSVC)
    set(CMAKE_INSTALL_OPENMP_LIBRARIES ON)
    set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION ${INSTALL_SHARED})
    include(InstallRequiredSystemLibraries)

elseif (UNIX)

    set(_thirdPartyLibs "stdc++")

    if (NOT "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        set(OpenMP_Lib "gomp")
        list(APPEND _thirdPartyLibs ${OpenMP_Lib})
    endif()

    set(_libsToDeploy)
    foreach(_lib ${_thirdPartyLibs})
        set(_optionName OPTION_DEPLOY_lib${_lib})
        option(${_optionName} "When deploying 3rd-party libraries, also deploy lib${_lib}." ON)
        mark_as_advanced(FORCE ${_optionName})
        if (${_optionName})
            list(APPEND _libsToDeploy ${_lib})
        endif()
    endforeach()

    if (NOT _libsToDeploy)
        return()
    endif()

    if(OpenMP_Lib AND OPTION_DEPLOY_lib${OpenMP_Lib})
        set(_deployOpenMP ON)
    endif()

    # Compile a simple C++ test program to locate third party libraries and determine their linked names.

    set(_testSourceFile "${CMAKE_CURRENT_BINARY_DIR}/cmake/linkcheck.cpp")
    set(_testCompileOptions)
    if (_deployOpenMP)
        set(_testSourceCode "#include <iostream>\nint main() {\n#pragma omp parallel\n{ std::cout<<\"\"<<std::endl\; }\nreturn 0\; }\n")
        list(APPEND _testCompileOptions "-fopenmp")
    else()
        set(_testSourceCode "#include <iostream>\nint main() {\n{ std::cout<<\"\"<<std::endl\; }\nreturn 0\; }\n")
    endif()
    set(_testBinary "${CMAKE_CURRENT_BINARY_DIR}/cmake/linkcheck.o")
    file(WRITE ${_testSourceFile} ${_testSourceCode})
    execute_process(
        COMMAND ${CMAKE_CXX_COMPILER} -O0 ${_testCompileOptions} -o ${_testBinary} ${_testSourceFile}
        RESULT_VARIABLE _returnValue
    )
    if (NOT _returnValue EQUAL 0)
        message(FATAL_ERROR "Could not compile a simple test program to determine linker parameters.")
    endif()


    # Extract library locations
    # generate something like: "lib(stdc\\+\\+|foo|bar)\\.so"
    set(_regexLocations "lib(")
    foreach(_lib ${_libsToDeploy})
        string(REPLACE "+" "\\+" _lib ${_lib})
        string(APPEND _regexLocations "${_lib}|")
    endforeach()
    string(LENGTH ${_regexLocations} _len)
    math(EXPR _len "${_len} - 1")
    string(SUBSTRING ${_regexLocations} 0 ${_len} _regexLocations)
    string(APPEND _regexLocations ")\\.so")

    execute_process(
        COMMAND bash "-c" "ldd ${_testBinary} | egrep '${_regexLocations}' | awk \'{ printf \"%s;%s\",$1,$3}\'"
        RESULT_VARIABLE _returnValue
        OUTPUT_VARIABLE _lddLibsInfo
    )
    if (NOT _returnValue EQUAL 0)
        message(FATAL_ERROR "Could not determine third party library locations.")
    endif()

    string(REPLACE "\n" ";" _lddLibsInfo "${_lddLibsInfo}")
    list(LENGTH _libsToDeploy _numLibsToDeploy)
    list(LENGTH _lddLibsInfo _lddOutputEntryCount)
    math(EXPR _expectedEntryCount1 "${_numLibsToDeploy} * 2")
    math(EXPR _expectedEntryCount2 "${_numLibsToDeploy} * 2 + 1") # including empty line at the end of the list
    if (NOT _expectedEntryCount1 EQUAL _lddOutputEntryCount AND NOT _expectedEntryCount2 EQUAL _lddOutputEntryCount)
        message(FATAL_ERROR "Unexpected ldd output. Cannot deploy third party libraries.")
    endif()

    math(EXPR _maxIdx "${_numLibsToDeploy} - 1")
    foreach(i RANGE ${_maxIdx})
        math(EXPR _libNameIdx "2 * ${i}")
        math(EXPR _libLocationIdx "2 * ${i} + 1")
        list(GET _lddLibsInfo ${_libNameIdx} _libName)
        list(GET _lddLibsInfo ${_libLocationIdx} _libLocation)

        # resolve symlinks
        get_filename_component(_libLocation ${_libLocation} REALPATH)
        install(FILES ${_libLocation}
            DESTINATION ${INSTALL_SHARED}
            RENAME ${_libName}
        )
    endforeach()

else()
    message(FATAL_ERROR "Cannot deploy system libraries for the current platform. (Not implemented)")
endif()
