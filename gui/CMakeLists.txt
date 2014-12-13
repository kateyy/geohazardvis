
set( TARGET gui )
message( STATUS ${TARGET} )

set ( SOURCES
    ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}_api.h

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
    data_view/RendererImplementation.h
    data_view/RendererImplementation.cpp
    data_view/RendererImplementationNull.h
    data_view/RendererImplementationNull.cpp
    data_view/RendererImplementation3D.h
    data_view/RendererImplementation3D.cpp
    data_view/RenderViewStrategy.h
    data_view/RenderViewStrategy.cpp
    data_view/RenderViewStrategyNull.h
    data_view/RenderViewStrategyNull.cpp
    data_view/RenderViewStrategy3D.h
    data_view/RenderViewStrategy3D.cpp
    data_view/RenderViewStrategyImage2D.h
    data_view/RenderViewStrategyImage2D.cpp
    data_view/RenderViewStrategyImage2DPlot.h
    data_view/RenderViewStrategyImage2DPlot.cpp
    data_view/TableView.h
    data_view/TableView.cpp

    propertyguizeug_extension/PropertyEditorFactoryEx.h
    propertyguizeug_extension/PropertyEditorFactoryEx.cpp    
    propertyguizeug_extension/PropertyPainterEx.h
    propertyguizeug_extension/PropertyPainterEx.cpp
    propertyguizeug_extension/ColorButtonWithBorder.h
    propertyguizeug_extension/ColorButtonWithBorder.cpp
    propertyguizeug_extension/ColorEditorRGB.h
    propertyguizeug_extension/ColorEditorRGB.cpp

    rendering_interaction/InteractorStyle3D.h
    rendering_interaction/InteractorStyle3D.cpp
    rendering_interaction/InteractorStyleImage.h
    rendering_interaction/InteractorStyleImage.cpp
    rendering_interaction/InteractorStyleSwitch.h
    rendering_interaction/InteractorStyleSwitch.cpp
    rendering_interaction/IPickingInteractorStyle.h
    rendering_interaction/PickingInteractorStyleSwitch.h
    rendering_interaction/PickingInteractorStyleSwitch.cpp

    widgets/CanvasExporterWidget.h
    widgets/CanvasExporterWidget.cpp
    widgets/DataBrowser.h
    widgets/DataBrowser.cpp
    widgets/DataBrowserTableModel.h
    widgets/DataBrowserTableModel.cpp
    widgets/ScalarMappingChooser.h
    widgets/ScalarMappingChooser.cpp
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
    
    widgets/CanvasExporterWidget.ui
    widgets/DataBrowser.ui
    widgets/ScalarMappingChooser.ui
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
source_group_by_path_and_type(${CMAKE_CURRENT_SOURCE_DIR} ${SOURCES} ${UIs} ${QRCs})
source_group_by_path(${CMAKE_CURRENT_BINARY_DIR} ".*" "Generated" ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}_api.h ${UI_Srcs} ${QRC_Srcs})
source_group_by_path(${CMAKE_CURRENT_SOURCE_DIR} ".*" "" "CMakeLists.txt")

if (OPTION_BUILD_STATIC)
    add_library(${TARGET} ${SOURCES})
else()
    add_library(${TARGET} SHARED ${SOURCES} ${UI_Srcs} ${QRC_Srcs})
endif()

target_link_libraries( ${TARGET}
    ${LIBZEUG_PROPERTYGUI_LIBRARY}
    ${LIBZEUG_REFLECTION_LIBRARY}
    ${LIBZEUG_SIGNAL_LIBRARY}
    ${VTK_LIBRARIES}
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


install(TARGETS ${TARGET}
    RUNTIME DESTINATION ${INSTALL_BIN}
    LIBRARY DESTINATION ${INSTALL_SHARED}
    # ARCHIVE DESTINATION ${INSTALL_LIB}
)
