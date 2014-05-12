
set( TARGET gui )
message( STATUS ${TARGET} )

set ( SOURCES
    inputviewer.h
    inputviewer.cpp
    mainwindow.h
    mainwindow.cpp
    normalrepresentation.h
    normalrepresentation.cpp
    pickinginteractionstyle.h
    pickinginteractionstyle.cpp
    qvtktablemodel.h
    qvtktablemodel.cpp
    selectionhandler.h
    selectionhandler.cpp
    tabwidget.h
    tabwidget.cpp
    widgets/datachooser.h
    widgets/datachooser.cpp
    widgets/renderconfigwidget.h
    widgets/renderconfigwidget.cpp
    widgets/renderview.h
    widgets/renderview.cpp
    widgets/tablewidget.h
    widgets/tablewidget.cpp
)

set ( UIs
    inputviewer.ui
    mainwindow.ui
    widgets/datachooser.ui
    widgets/renderconfigwidget.ui
    widgets/renderview.ui
    widgets/tablewidget.ui
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


add_library ( ${TARGET}
    ${SOURCES} ${UI_Srcs} )

qt5_use_modules( ${TARGET} Core Gui Widgets )

target_link_libraries( ${TARGET}
    ${VTK_LIBRARIES}
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
    LINK_FLAGS_DEBUG                    "${DEFAULT_LINKER_FLAGS_DEBUG}"
    LINK_FLAGS_RELEASE                  "${DEFAULT_LINKER_FLAGS_RELEASE}"
    LINK_FLAGS_RELWITHDEBINFO           "${DEFAULT_LINKER_FLAGS_RELEASE}"
    LINK_FLAGS_MINSIZEREL               "${DEFAULT_LINKER_FLAGS_RELEASE}"
)
