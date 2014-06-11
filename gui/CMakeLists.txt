
set( TARGET gui )
message( STATUS ${TARGET} )

set ( SOURCES
    ${TARGET}_api.h
    
    DataMapping.h
    DataMapping.cpp
    MainWindow.h
    MainWindow.cpp
    NormalRepresentation.h
    NormalRepresentation.cpp
    PickingInteractionStyle.h
    PickingInteractionStyle.cpp
    SelectionHandler.h
    SelectionHandler.cpp
    widgets/DataChooser.h
    widgets/DataChooser.cpp
    widgets/RenderConfigWidget.h
    widgets/RenderConfigWidget.cpp
    widgets/RenderWidget.h
    widgets/RenderWidget.cpp
    widgets/TableWidget.h
    widgets/TableWidget.cpp
)

set ( UIs
    MainWindow.ui
    widgets/DataChooser.ui
    widgets/RenderConfigWidget.ui
    widgets/RenderWidget.ui
    widgets/TableWidget.ui
)


find_package(Qt5Widgets REQUIRED QUIET)
qt5_wrap_ui(UI_Srcs ${UIs})
#qt5_add_resources(QRC_Srcs ${QRCs} )


# Group source files
source_group_by_path( ${CMAKE_CURRENT_SOURCE_DIR} "\\\\.cpp$|\\\\.c$|\\\\.h$|\\\\.hpp$|\\\\.inl$"
    "Source Files" ${SOURCES} )
source_group_by_path( ${CMAKE_CURRENT_SOURCE_DIR} "\\\\.ui$"
    "Ressources" ${UIs} )
source_group_by_path( ${CMAKE_CURRENT_BINARY_DIR} "\\\\.cpp$|\\\\.c$|\\\\.h$|\\\\.hpp$|\\\\.inl$"
    "Generated" ${UI_Srcs} ${MOC_Srcs} )

if (OPTION_BUILD_STATIC)
    add_library(${TARGET} ${SOURCES})
else()
    add_library(${TARGET} SHARED ${SOURCES} ${UI_Srcs})
endif()

target_link_libraries( ${TARGET}
    ${VTK_LIBRARIES}
    ${LIBZEUG_SIGNAL_LIBRARY}
    ${LIBZEUG_REFLECTION_LIBRARY}
    ${LIBZEUG_PROPERTYGUI_LIBRARY}
    Qt5::Widgets
    
    core
)

include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${LIBZEUG_SIGNAL_INCLUDE_DIR}
    ${LIBZEUG_REFLECTION_INCLUDE_DIR}
    ${LIBZEUG_PROPERTYGUI_INCLUDE_DIR}
)

set_target_properties( ${TARGET}
    PROPERTIES
    LINKER_LANGUAGE CXX
    COMPILE_DEFINITIONS_DEBUG           "${DEFAULT_COMPILE_DEFS_DEBUG}"
    COMPILE_DEFINITIONS_RELEASE         "${DEFAULT_COMPILE_DEFS_RELEASE}"
    COMPILE_DEFINITIONS_RELWITHDEBINFO  "${DEFAULT_COMPILE_DEFS_RELEASE}"
    COMPILE_DEFINITIONS_MINSIZEREL      "${DEFAULT_COMPILE_DEFS_RELEASE}"
    COMPILE_FLAGS                       "${DEFAULT_COMPILE_FLAGS}"
    LINK_FLAGS_DEBUG                    "${DEFAULT_LINKER_FLAGS_DEBUG}"
    LINK_FLAGS_RELEASE                  "${DEFAULT_LINKER_FLAGS_RELEASE}"
    LINK_FLAGS_RELWITHDEBINFO           "${DEFAULT_LINKER_FLAGS_RELEASE}"
    LINK_FLAGS_MINSIZEREL               "${DEFAULT_LINKER_FLAGS_RELEASE}"
)

generate_library_export_header(${TARGET})
