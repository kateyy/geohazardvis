
include(CppCheckTargets)

include(CMakeParseArguments)    # include not required in CMake 3.5+

set(PLUGIN_TARGETS_DOC_STRING "Ensure that the main executable is always run with up to date plugin libraries.")
set(PLUGIN_TARGETS "" CACHE INTERNAL ${PLUGIN_TARGETS_DOC_STRING})

function(configure_cxx_target target)

    set(options PLUGIN_TARGET AUX_PLUGIN_TARGET NO_CPPCHECK)
    set(oneValueArgs IDE_FOLDER)
    set(multiValueArgs)

    cmake_parse_arguments("option"
        "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if (NOT option_IDE_FOLDER)
        set(option_IDE_FOLDER ${IDE_FOLDER})
    endif()

    target_compile_definitions(${target} PRIVATE ${DEFAULT_COMPILE_DEFS} ${VTK_DEFINITIONS})

    target_compile_options(${target} PRIVATE ${DEFAULT_COMPILE_FLAGS})

    set_target_properties(${target}
        PROPERTIES
        LINKER_LANGUAGE CXX
        DEBUG_POSTFIX                          "_d${DEBUG_POSTFIX}"
        RELWITHDEBINFO_POSTFIX                 "_rd${DEBUG_POSTFIX}"
        RELNOOPTIMIZATION_POSTFIX              "_rd0${DEBUG_POSTFIX}"
        FOLDER                                 "${option_IDE_FOLDER}"
    )

    if (DEFAULT_LINKER_FLAGS_DEBUG OR DEFAULT_LINKER_FLAGS_RELEASE)
        # MSVC flags not properly handled by target_link_libraries, fall-back to legacy is required
        set_target_properties(${target}
            PROPERTIES
            LINK_FLAGS_DEBUG                       "${DEFAULT_LINKER_FLAGS_DEBUG}"
            LINK_FLAGS_RELEASE                     "${DEFAULT_LINKER_FLAGS_RELEASE}"
            LINK_FLAGS_RELWITHDEBINFO              "${DEFAULT_LINKER_FLAGS_RELWITHDEBINFO}"
            LINK_FLAGS_RELNOOPTIMIZATION           "${DEFAULT_LINKER_FLAGS_RELNOOPTIMIZATION}"
            STATIC_LIBRARY_FLAGS_DEBUG             "${DEFAULT_STATIC_LIBRARY_FLAGS_DEBUG}"
            STATIC_LIBRARY_FLAGS_RELEASE           "${DEFAULT_STATIC_LIBRARY_FLAGS_RELEASE}"
            STATIC_LIBRARY_FLAGS_RELWITHDEBINFO    "${DEFAULT_STATIC_LIBRARY_FLAGS_RELWITHDEBINFO}"
            STATIC_LIBRARY_FLAGS_RELNOOPTIMIZATION "${DEFAULT_STATIC_LIBRARY_FLAGS_RELNOOPTIMIZATION}"
        )
    else()
        target_link_libraries(${target} ${DEFAULT_LINKER_FLAGS})
    endif()

    if (NOT CMAKE_VERSION VERSION_LESS 3.1)
        set_target_properties(${target}
            PROPERTIES
            CXX_STANDARD 14
            CXX_STANDARD_REQUIRED ON
        )
    endif()

    if (NOT option_NO_CPPCHECK)
        cppcheck_target(${target})
    endif()

    if (option_AUX_PLUGIN_TARGET OR option_PLUGIN_TARGET)
        # TODO replace by generator expressions, once requiring CMake 3.4
        if(generatorIsMultiConfig)
            foreach(_config ${ACCEPTED_CONFIGURATION_TYPES})
                string(TOUPPER ${_config} _configUpper)
                set_target_properties(${target}
                    PROPERTIES
                    RUNTIME_OUTPUT_DIRECTORY_${_configUpper} "${CMAKE_BINARY_DIR}/${_config}/plugins"
                    LIBRARY_OUTPUT_DIRECTORY_${_configUpper} "${CMAKE_BINARY_DIR}/${_config}/plugins"
                    ARCHIVE_OUTPUT_DIRECTORY_${_configUpper} "${CMAKE_BINARY_DIR}/${_config}/plugins"
                )
            endforeach()
        else()
            set_target_properties(${target}
                PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
                LIBRARY_OUTPUT_DIRECTORY "${RUNTIME_OUTPUT_DIRECTORY}"
                ARCHIVE_OUTPUT_DIRECTORY "${RUNTIME_OUTPUT_DIRECTORY}"
            )
        endif()
    endif()

    if (option_PLUGIN_TARGET)
        set(PLUGIN_TARGETS "${PLUGIN_TARGETS};${target}" CACHE INTERNAL ${PLUGIN_TARGETS_DOC_STRING})
    endif()

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
