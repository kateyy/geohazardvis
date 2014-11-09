
function(configure_cxx_target TARGET)

    target_compile_definitions(${TARGET} PRIVATE ${DEFAULT_COMPILE_DEFS})

    target_compile_options(${TARGET} PRIVATE ${DEFAULT_COMPILE_FLAGS})
    
    set_target_properties( ${TARGET}
        PROPERTIES
        LINKER_LANGUAGE CXX
        LINK_FLAGS_DEBUG                    "${DEFAULT_LINKER_FLAGS_DEBUG}"
        LINK_FLAGS_RELEASE                  "${DEFAULT_LINKER_FLAGS_RELEASE}"
        LINK_FLAGS_RELWITHDEBINFO           "${DEFAULT_LINKER_FLAGS_RELEASE}"
        LINK_FLAGS_MINSIZEREL               "${DEFAULT_LINKER_FLAGS_RELEASE}"
        DEBUG_POSTFIX                       "d${DEBUG_POSTFIX}"
        RELWITHDEBINFO_POSTFIX              "rd${DEBUG_POSTFIX}"
    )
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

    if (IDE_SPLIT_HEADERS_SOURCES)
        source_group_by_path(${PARENT_PATH} ".*[.](h|hpp)$" "Header Files" ${ARGN})
        source_group_by_path(${PARENT_PATH} ".*[.](c|cpp)$" "Source Files" ${ARGN})
    else()
        source_group_by_path(${PARENT_PATH} ".*[.](cpp|c|h|hpp)" "Source Files" ${ARGN})
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
