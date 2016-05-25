
if (MSVC)
    set(CMAKE_INSTALL_OPENMP_LIBRARIES ON)
    set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION ${INSTALL_SHARED})
    include(InstallRequiredSystemLibraries)

elseif (UNIX)
    # Compile a simple C++ test program to locate libstdc++ and libc and deploy them.

    option(OPTION_DEPLOY_LIBSTDCXX "When deploying 3rd-party libraries, also deploy libstdc++." ON)
    if (NOT OPTION_DEPLOY_LIBSTDCXX)
        return()
    endif()

    set(_testSourceFile "${CMAKE_CURRENT_BINARY_DIR}/cmake/minimal.cpp")
    set(_testSourceCode "#include <iostream>\nint main() { return 0\; }\n")
    set(_testBinary "${CMAKE_CURRENT_BINARY_DIR}/cmake/minimal.o")
    file(WRITE ${_testSourceFile} ${_testSourceCode})
    execute_process(
        COMMAND ${CMAKE_CXX_COMPILER} -O0 -o ${_testBinary} ${_testSourceFile}
        RESULT_VARIABLE _returnValue
    )
    if (NOT _returnValue EQUAL 0)
        message(FATAL_ERROR "Could not compile a simple test program to determine linker parameters.")
    endif()
    execute_process(
       # COMMAND bash "-c" "ldd ${_testBinary} | egrep 'lib(stdc\\+\\+|c)\\.so' | awk \'{ print $1\"\;\"$3 }\'"
        COMMAND bash "-c" "ldd ${_testBinary} | grep libstdc++.so | awk \'{ print $1\"\;\"$3 }\'"
        RESULT_VARIABLE _returnValue
        OUTPUT_VARIABLE _lddLibsInfo
    )
    if (NOT _returnValue EQUAL 0)
        message(FATAL_ERROR "Could not determine libstdc and libstdc++ library locations.")
    endif()

    string(REPLACE "\n" ";" _lddLibsInfo ${_lddLibsInfo})
    list(GET _lddLibsInfo 0 _lib1name)
    list(GET _lddLibsInfo 1 _lib1location)
   # list(GET _lddLibsInfo 2 _lib2name)
   # list(GET _lddLibsInfo 3 _lib2location)

    # resolve symlinks
    get_filename_component(_lib1location ${_lib1location} REALPATH)
   # get_filename_component(_lib2location ${_lib2location} REALPATH)

    install(FILES ${_lib1location}
        DESTINATION ${INSTALL_SHARED}
        RENAME ${_lib1name}
    )
   # install(FILES ${_lib2location}
   #     DESTINATION ${INSTALL_SHARED}
   #     RENAME ${_lib2name}
   # )
else()
    message(FATAL_ERROR "Cannot deploy system libraries for the current platform. (Not implemented)")
endif()
