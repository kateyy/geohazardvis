
set( TARGET testing )
message( STATUS ${TARGET} )

set ( SOURCES
    main.cpp
)
    
source_group_by_path_and_type(${CMAKE_CURRENT_SOURCE_DIR} ${SOURCES})

add_executable ( ${TARGET} ${SOURCES} )

target_link_libraries( ${TARGET}
    ${VTK_LIBRARIES}

    core
)


configure_cxx_target( ${TARGET} )

install(TARGETS ${TARGET}
    RUNTIME DESTINATION ${INSTALL_BIN}
#   LIBRARY DESTINATION ${INSTALL_SHARED}
#   ARCHIVE DESTINATION ${INSTALL_LIB}
)
