
function(writeBinaryVersionInfo target)

    if (NOT WIN32)
        return()
    endif()

    set(scriptFile "${PROJECT_SOURCE_DIR}/cmake/GenerateBinaryVersionResource.cmake")
    set(inFile "${PROJECT_SOURCE_DIR}/cmake/windows-binary-version.rc.in")
    set(outFile "${CMAKE_CURRENT_BINARY_DIR}/version.rc")

    get_target_property(targetType ${target} TYPE)

    add_custom_command(TARGET ${target} PRE_BUILD
        COMMAND ${CMAKE_COMMAND}
            -D rcInFile=${inFile}
            -D rcOutFile=${outFile}
            -D targetType=${targetType}
            -D targetRuntimeFileName=$<TARGET_FILE_NAME:${target}>
            -D PRODUCTNAME=${CMAKE_PROJECT_NAME}
            -D FILEDESCRIPTION=${CMAKE_PROJECT_NAME}
            -D PROJECT_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
            -D PROJECT_VERSION_MINOR=${PROJECT_VERSION_MINOR}
            -D PROJECT_VERSION_PATCH=${PROJECT_VERSION_PATCH}
            -D PROJECT_VERSION_TWEAK=${PROJECT_VERSION_TWEAK}
            -D LEGALCOPYRIGHT=${META_AUTHOR_MAINTAINER}
            -D GIT_SHA1=${GIT_SHA1}
            -D GIT_DATE=${GIT_DATE}
            -P ${scriptFile}
        BYPRODUCT ${outFile}
    )

    if (NOT EXISTS ${outFile})
        file(WRITE ${outFile} "")
    endif()

    get_target_property(sources ${target} SOURCES)
    list(APPEND sources ${outFile})
    set_target_properties(${target}
        PROPERTIES SOURCES "${sources}"
    )

endfunction()
