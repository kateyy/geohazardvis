
set( TARGET gui )
message( STATUS ${TARGET} )

set ( SOURCES
    viewer.cpp
    viewer.h
    pickinginteractionstyle.cpp
    pickinginteractionstyle.h
    pickinginfo.h
    pickinginfo.cpp )
    
set ( UIs
    viewer.ui )
    

find_package(Qt5Widgets REQUIRED QUIET)
qt5_wrap_ui(UI_Srcs ${UIs})
#qt5_add_resources(QRC_Srcs ${QRCs} )

source_group("Source Files" FILES
    ${SOURCES}
)

source_group("Resources" FILES
    ${UIs}
    #${QRCs}
)

source_group("Generated" FILES
    ${UI_Srcs}
    ${MOC_Srcs}
    #${QRC_Srcs}
)

add_library ( ${TARGET}
    ${SOURCES} ${UI_Srcs} )

qt5_use_modules( ${TARGET} Core Gui Widgets OpenGL OpenGLExtensions )

target_link_libraries( ${TARGET}
    ${VTK_LIBRARIES}
    core
)
    
include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
)

set_target_properties( ${TARGET}
    PROPERTIES
    LINKER_LANGUAGE CXX
    COMPILE_DEFINITIONS_DEBUG   "${DEFAULT_COMPILE_DEFS_DEBUG}"
    COMPILE_DEFINITIONS_RELEASE "${DEFAULT_COMPILE_DEFS_RELEASE}"
    LINK_FLAGS_DEBUG            "${DEFAULT_LINKER_FLAGS_DEBUG}"
    LINK_FLAGS_RELEASE          "${DEFAULT_LINKER_FLAGS_RELEASE}" )
