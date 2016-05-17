
include(CppCheckTargets)

function(configure_cxx_target TARGET)

    target_compile_definitions(${TARGET} PRIVATE ${DEFAULT_COMPILE_DEFS})

    target_compile_options(${TARGET} PRIVATE ${DEFAULT_COMPILE_FLAGS})

    set_target_properties( ${TARGET}
        PROPERTIES
        LINKER_LANGUAGE CXX
        LINK_FLAGS_DEBUG                    "${DEFAULT_LINKER_FLAGS_DEBUG}"
        LINK_FLAGS_RELEASE                  "${DEFAULT_LINKER_FLAGS_RELEASE}"
        LINK_FLAGS_RELWITHDEBINFO           "${DEFAULT_LINKER_FLAGS_RELWITHDEBINFO}"
        LINK_FLAGS_RELNOOPTIMIZATION        "${DEFAULT_LINKER_FLAGS_RELNOOPTIMIZATION}"
        DEBUG_POSTFIX                       "_d${DEBUG_POSTFIX}"
        RELWITHDEBINFO_POSTFIX              "_rd${DEBUG_POSTFIX}"
        RELNOOPTIMIZATION_POSTFIX           "_rd0${DEBUG_POSTFIX}"
        FOLDER                              "${IDE_FOLDER}"
    )

    cppcheck_target(${TARGET})

endfunction()

set(PLUGIN_TARGETS_DOC_STRING "Ensure that the main executable is always run with up to date plugin libraries.")
set(PLUGIN_TARGETS "" CACHE INTERNAL ${PLUGIN_TARGETS_DOC_STRING})

function(configure_cxx_plugin TARGET)

    configure_cxx_target(${TARGET})

    if(MSVC) # for multi-configuration generators
        set_target_properties(${TARGET}
            PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/Debug/plugins"
            RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/Release/plugins"
            RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/RelWithDebInfo/plugins"
            RUNTIME_OUTPUT_DIRECTORY_RELNOOPTIMIZATION "${CMAKE_BINARY_DIR}/RelNoOptimization/plugins"
        )
    else()
        set_target_properties(${TARGET}
            PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
        )
    endif()

    set(PLUGIN_TARGETS "${PLUGIN_TARGETS};${TARGET}" CACHE INTERNAL ${PLUGIN_TARGETS_DOC_STRING})

endfunction()

# Define function "source_group_by_path with three mandatory arguments (PARENT_PATH, REGEX, GROUP, ...)
# to group source files in folders (e.g. for MSVC solutions).
#
# Example:
# source_group_by_path("${CMAKE_CURRENT_SOURCE_DIR}/src" "\\\\.h$|\\\\.hpp$|\\\\.cpp$|\\\\.c$|\\\\.ui$|\\\\.qrc$" "Source Files" ${sources})
function (source_group_by_path PARENT_PATH REGEX GROUP)

    foreach (FILENAME ${ARGN})

        string(REGEX MATCH ${REGEX} regex_match ${FILENAME})

        if (regex_match)
            get_filename_component(FILEPATH "${FILENAME}" REALPATH)
            file(RELATIVE_PATH FILEPATH ${PARENT_PATH} ${FILEPATH})
            get_filename_component(FILEPATH "${FILEPATH}" DIRECTORY)

            string(REPLACE "/" "\\" FILEPATH "${FILEPATH}")

            source_group("${GROUP}\\${FILEPATH}" REGULAR_EXPRESSION "${REGEX}" FILES ${FILENAME})
        endif()

    endforeach()

endfunction (source_group_by_path)

function (source_group_by_path_and_type PARENT_PATH)

    if (OPTION_IDE_SPLIT_HEADERS_SOURCES)
        source_group_by_path(${PARENT_PATH} ".*[.](h|hpp)$" "Header Files" ${ARGN})
        source_group_by_path(${PARENT_PATH} ".*[.](c|cpp|cxx)$" "Source Files" ${ARGN})
    else()
        source_group_by_path(${PARENT_PATH} ".*[.](cpp|cxx|c|h|hpp)" "Source Files" ${ARGN})
    endif()

    source_group_by_path(${PARENT_PATH} ".*[.](qrc|ui)$" "Ressources" ${ARGN})

endfunction(source_group_by_path_and_type)


include (GenerateExportHeader)

function(generate_library_export_header LIBNAME)

    string(TOUPPER ${LIBNAME}_API LIBRARY_EXPORT_MACRO)

    generate_export_header( ${LIBNAME}
        BASE_NAME ${LIBNAME}
        EXPORT_MACRO_NAME ${LIBRARY_EXPORT_MACRO}
        EXPORT_FILE_NAME ${LIBNAME}_api.h
        STATIC_DEFINE OPTION_BUILD_STATIC)

endfunction()

# see also: CMakeDependentOption.cmake
macro(CMAKE_DEPENDENT_CACHEVARIABLE name default type doc depends force)
  if(${name}_ISSET MATCHES "^${name}_ISSET$")
    set(${name}_AVAILABLE 1)
    foreach(d ${depends})
      string(REGEX REPLACE " +" ";" CMAKE_DEPENDENT_OPTION_DEP "${d}")
      if(${CMAKE_DEPENDENT_OPTION_DEP})
      else()
        set(${name}_AVAILABLE 0)
      endif()
    endforeach()
    if(${name}_AVAILABLE)
      set(${name} "${default}" CACHE ${type} "${doc}") # create if does not exist
      set(${name} "${${name}}" CACHE ${type} "${doc}" FORCE)   # force to non-internal
    else()
      if(${name} MATCHES "^${name}$")
      else()
        set(${name} "${${name}}" CACHE INTERNAL "${doc}")
      endif()
      set(${name} ${force})
    endif()
  else()
    set(${name} "${${name}_ISSET}")
  endif()
endmacro()
