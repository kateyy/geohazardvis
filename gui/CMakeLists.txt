
set( TARGET gui )
message( STATUS ${TARGET} )

set ( SOURCES
    ${TARGET}_api.h

    DataMapping.h
    DataMapping.cpp
    MainWindow.h
    MainWindow.cpp
    SelectionHandler.h
    SelectionHandler.cpp

    data_view/AbstractDataView.h
    data_view/AbstractDataView.cpp
    data_view/RenderViewSwitch.h
    data_view/RenderViewSwitch.cpp
    data_view/RenderView.h
    data_view/RenderView.cpp
    data_view/RenderViewStrategy.h
    data_view/RenderViewStrategy.cpp
    data_view/RenderViewStrategyNull.h
    data_view/RenderViewStrategyNull.cpp
    data_view/RenderViewStrategy3D.h
    data_view/RenderViewStrategy3D.cpp
    data_view/RenderViewStrategyImage2D.h
    data_view/RenderViewStrategyImage2D.cpp
    data_view/TableView.h
    data_view/TableView.cpp

    rendering_interaction/InteractorStyle3D.h
    rendering_interaction/InteractorStyle3D.cpp
    rendering_interaction/InteractorStyleImage.h
    rendering_interaction/InteractorStyleImage.cpp
    rendering_interaction/InteractorStyleSwitch.h
    rendering_interaction/InteractorStyleSwitch.cpp
    rendering_interaction/IPickingInteractorStyle.h
    rendering_interaction/PickingInteractorStyleSwitch.h
    rendering_interaction/PickingInteractorStyleSwitch.cpp

    widgets/DataBrowser.h
    widgets/DataBrowser.cpp
    widgets/DataBrowserTableModel.h
    widgets/DataBrowserTableModel.cpp
    widgets/ScalarMappingChooser.h
    widgets/ScalarMappingChooser.cpp
    widgets/ScalarRearrangeObjects.h
    widgets/ScalarRearrangeObjects.cpp
    widgets/RenderConfigWidget.h
    widgets/RenderConfigWidget.cpp
    widgets/RendererConfigWidget.h
    widgets/RendererConfigWidget.cpp
    widgets/VectorMappingChooser.h
    widgets/VectorMappingChooser.cpp
    widgets/VectorMappingChooserListModel.h
    widgets/VectorMappingChooserListModel.cpp
)

set ( UIs
    MainWindow.ui
    
    data_view/RenderView.ui
    data_view/TableView.ui
    
    widgets/DataBrowser.ui
    widgets/ScalarMappingChooser.ui
    widgets/ScalarRearrangeObjects.ui
    widgets/RenderConfigWidget.ui
    widgets/RendererConfigWidget.ui
    widgets/VectorMappingChooser.ui
)

set ( QRCs
    icons.qrc
)

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
