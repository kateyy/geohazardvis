
set(target ${META_PROJECT_NAME})
message(STATUS ${target})

set(sources
    main.cpp
    Application.h
    Application.cpp )

source_group_by_path_and_type(${CMAKE_CURRENT_SOURCE_DIR} ${sources})

add_executable (${target} ${WIN32_EXE_FLAG} ${sources})

target_link_libraries(${target}
    PUBLIC
    gui
)

if (PLUGIN_TARGETS)
    add_dependencies(${target} ${PLUGIN_TARGETS})
endif()

configure_cxx_target(${target})
setupProjectUserConfig(${target})
writeBinaryVersionInfo(${target})

install(TARGETS ${target}
    RUNTIME DESTINATION ${INSTALL_BIN}
#   LIBRARY DESTINATION ${INSTALL_SHARED}
#   ARCHIVE DESTINATION ${INSTALL_LIB}
)
