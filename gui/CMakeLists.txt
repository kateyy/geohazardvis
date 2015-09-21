
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
    data_view/AbstractRenderView.h
    data_view/AbstractRenderView.cpp
    data_view/RenderViewStrategySwitch.h
    data_view/RenderViewStrategySwitch.cpp
    data_view/RenderView.h
    data_view/RenderView.cpp
    data_view/RendererImplementation.h
    data_view/RendererImplementation.cpp
    data_view/RendererImplementationNull.h
    data_view/RendererImplementationNull.cpp
    data_view/RendererImplementation3D.h
    data_view/RendererImplementation3D.cpp
    data_view/RendererImplementationBase3D.h
    data_view/RendererImplementationBase3D.cpp
    data_view/RendererImplementationPlot.h
    data_view/RendererImplementationPlot.cpp
    data_view/RendererImplementationResidual.h
    data_view/RendererImplementationResidual.cpp
    data_view/RendererImplementationSwitch.h
    data_view/RendererImplementationSwitch.cpp
    data_view/RenderViewStrategy.h
    data_view/RenderViewStrategy.cpp
    data_view/RenderViewStrategyNull.h
    data_view/RenderViewStrategyNull.cpp
    data_view/RenderViewStrategy3D.h
    data_view/RenderViewStrategy3D.cpp
    data_view/RenderViewStrategyImage2D.h
    data_view/RenderViewStrategyImage2D.cpp
    data_view/ResidualVerificationView.h
    data_view/ResidualVerificationView.cpp
    data_view/TableView.h
    data_view/TableView.cpp

    propertyguizeug_extension/ColorButtonWithBorder.h
    propertyguizeug_extension/ColorButtonWithBorder.cpp
    propertyguizeug_extension/ColorEditorRGB.h
    propertyguizeug_extension/ColorEditorRGB.cpp

    rendering_interaction/CameraDolly.h
    rendering_interaction/CameraDolly.cpp
    rendering_interaction/InteractorStyle3D.h
    rendering_interaction/InteractorStyle3D.cpp
    rendering_interaction/InteractorStyleImage.h
    rendering_interaction/InteractorStyleImage.cpp
    rendering_interaction/InteractorStyleSwitch.h
    rendering_interaction/InteractorStyleSwitch.cpp
    rendering_interaction/Highlighter.h
    rendering_interaction/Highlighter.cpp
    rendering_interaction/IPickingInteractorStyle.h
    rendering_interaction/Picker.h
    rendering_interaction/Picker.cpp
    rendering_interaction/PickingInteractorStyleSwitch.h
    rendering_interaction/PickingInteractorStyleSwitch.cpp

    widgets/CanvasExporterWidget.h
    widgets/CanvasExporterWidget.cpp
    widgets/ColorMappingChooser.h
    widgets/ColorMappingChooser.cpp
    widgets/CollapsibleGroupBox.h
    widgets/CollapsibleGroupBox.cpp
    widgets/DataBrowser.h
    widgets/DataBrowser.cpp
    widgets/DataBrowserTableModel.h
    widgets/DataBrowserTableModel.cpp
    widgets/DEMWidget.h
    widgets/DEMWidget.cpp
    widgets/GlyphMappingChooser.h
    widgets/GlyphMappingChooser.cpp
    widgets/GlyphMappingChooserListModel.h
    widgets/GlyphMappingChooserListModel.cpp
    widgets/RenderConfigWidget.h
    widgets/RenderConfigWidget.cpp
    widgets/RendererConfigWidget.h
    widgets/RendererConfigWidget.cpp
    widgets/RendererConfigWidget_Renderer.cpp
)

set ( UIs
    MainWindow.ui

    data_view/RenderView.ui
    data_view/TableView.ui

    widgets/CanvasExporterWidget.ui
    widgets/ColorMappingChooser.ui
    widgets/DataBrowser.ui
    widgets/DEMWidget.ui
    widgets/GlyphMappingChooser.ui
    widgets/RenderConfigWidget.ui
    widgets/RendererConfigWidget.ui
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
    Qt5::Widgets

    core
)

include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${LIBZEUG_INCLUDES}
)


configure_cxx_target( ${TARGET} )

generate_library_export_header(${TARGET})


install(TARGETS ${TARGET}
    RUNTIME DESTINATION ${INSTALL_BIN}
    LIBRARY DESTINATION ${INSTALL_SHARED}
    # ARCHIVE DESTINATION ${INSTALL_LIB}
)
