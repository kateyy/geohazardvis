
set(TARGET ${META_PROJECT_NAME})
message(STATUS ${TARGET})

set (SOURCES
    main.cpp
    Application.h
    Application.cpp )

source_group_by_path_and_type(${CMAKE_CURRENT_SOURCE_DIR} ${SOURCES})

add_executable (${TARGET} ${WIN32_EXE_FLAG} ${SOURCES})

target_link_libraries(${TARGET}
    gui
)

if (PLUGIN_TARGETS)
    add_dependencies(${TARGET} ${PLUGIN_TARGETS})
endif()

configure_cxx_target( ${TARGET} )
setupProjectUserConfig(${TARGET})

install(TARGETS ${TARGET}
    RUNTIME DESTINATION ${INSTALL_BIN}
#   LIBRARY DESTINATION ${INSTALL_SHARED}
#   ARCHIVE DESTINATION ${INSTALL_LIB}
)
