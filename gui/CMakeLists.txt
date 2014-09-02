
set( TARGET gui )
message( STATUS ${TARGET} )

set ( SOURCES
    ${TARGET}_api.h
    
    DataMapping.h
    DataMapping.cpp
    InteractorStyle3D.h
    InteractorStyle3D.cpp
    InteractorStyleImage.h
    InteractorStyleImage.cpp
    InteractorStyleSwitch.h
    InteractorStyleSwitch.cpp
    IPickingInteractorStyle.h
    PickingInteractorStyleSwitch.h
    PickingInteractorStyleSwitch.cpp
    MainWindow.h
    MainWindow.cpp
    SelectionHandler.h
    SelectionHandler.cpp
    widgets/DataBrowser.h
    widgets/DataBrowser.cpp
    widgets/DataBrowserTableModel.h
    widgets/DataBrowserTableModel.cpp
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
    widgets/DataBrowser.ui
    widgets/DataChooser.ui
    widgets/RenderConfigWidget.ui
    widgets/RenderWidget.ui
    widgets/TableWidget.ui
)

set ( QRCs
    icons.qrc
)

find_package(Qt5Widgets REQUIRED QUIET)
qt5_wrap_ui(UI_Srcs ${UIs})
qt5_add_resources(QRC_Srcs ${QRCs} )


# Group source files
source_group_by_path( ${CMAKE_CURRENT_SOURCE_DIR} "\\\\.cpp$|\\\\.c$|\\\\.h$|\\\\.hpp$|\\\\.inl$"
    "Source Files" ${SOURCES} )
source_group_by_path( ${CMAKE_CURRENT_SOURCE_DIR} "\\\\.ui$|\\\\.qrc$"
    "Ressources" ${UIs} ${QRCs})
source_group_by_path( ${CMAKE_CURRENT_BINARY_DIR} "\\\\.cpp$|\\\\.c$|\\\\.h$|\\\\.hpp$|\\\\.inl$"
    "Generated" ${UI_Srcs} ${MOC_Srcs} ${QRC_Srcs})

if (OPTION_BUILD_STATIC)
    add_library(${TARGET} ${SOURCES})
else()
    add_library(${TARGET} SHARED ${SOURCES} ${UI_Srcs} ${QRC_Srcs})
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


configure_cxx_target( ${TARGET} )

generate_library_export_header(${TARGET})
