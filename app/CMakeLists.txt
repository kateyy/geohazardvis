
set( TARGET ${META_PROJECT_NAME} )
message( STATUS ${TARGET} )

set ( SOURCES
    main.cpp
    application.h
    application.cpp )

add_executable ( ${TARGET} ${SOURCES} )

qt5_use_modules( ${TARGET} Widgets )

target_link_libraries( ${TARGET}
    core
    gui )
    
set_target_properties( ${TARGET}
    PROPERTIES
    LINKER_LANGUAGE CXX
    COMPILE_DEFINITIONS_DEBUG   "${DEFAULT_COMPILE_DEFS_DEBUG}"
    COMPILE_DEFINITIONS_RELEASE "${DEFAULT_COMPILE_DEFS_RELEASE}"
    LINK_FLAGS_DEBUG            "${DEFAULT_LINKER_FLAGS_DEBUG}"
    LINK_FLAGS_RELEASE          "${DEFAULT_LINKER_FLAGS_RELEASE}" )
