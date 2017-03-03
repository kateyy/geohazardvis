
set(target gui)
message(STATUS ${target})

set(sources
    ${CMAKE_CURRENT_BINARY_DIR}/${target}_api.h

    DataMapping.h
    DataMapping.cpp
    MainWindow.h
    MainWindow.cpp
    SelectionHandler.h
    SelectionHandler.cpp

    data_handling/CoordinateSystemAdjustmentWidget.h
    data_handling/CoordinateSystemAdjustmentWidget.cpp
    data_handling/DataBrowser.h
    data_handling/DataBrowser.cpp
    data_handling/DataBrowserTableModel.h
    data_handling/DataBrowserTableModel.cpp
    data_handling/DEMWidget.h
    data_handling/DEMWidget.cpp

    data_view/AbstractDataView.h
    data_view/AbstractDataView.cpp
    data_view/AbstractRenderView.h
    data_view/AbstractRenderView.cpp
    data_view/ChartXY.h
    data_view/ChartXY.cpp
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
    data_view/RenderViewStrategy2D.h
    data_view/RenderViewStrategy2D.cpp
    data_view/RenderViewStrategy3D.h
    data_view/RenderViewStrategy3D.cpp
    data_view/ResidualVerificationView.h
    data_view/ResidualVerificationView.cpp
    data_view/t_QVTKWidget.h
    data_view/t_QVTKWidgetFwd.h
    data_view/t_QVTKWidget.cpp
    data_view/TableView.h
    data_view/TableView.cpp

    io/CanvasExporterWidget.h
    io/CanvasExporterWidget.cpp
    io/DataImporterWidget.h
    io/DataImporterWidget.cpp
    io/GridDataImporterWidget.h
    io/GridDataImporterWidget.cpp

    plugin/gui_plugin_api.h
    plugin/GuiPlugin.h
    plugin/GuiPlugin.cpp
    plugin/GuiPluginInterface.h
    plugin/GuiPluginInterface.cpp
    plugin/GuiPluginLibrary.h
    plugin/GuiPluginLibrary.cpp
    plugin/GuiPluginManager.h
    plugin/GuiPluginManager.cpp

    propertyguizeug_extension/ColorButtonWithBorder.h
    propertyguizeug_extension/ColorButtonWithBorder.cpp
    propertyguizeug_extension/ColorEditorRGB.h
    propertyguizeug_extension/ColorEditorRGB.cpp

    rendering_interaction/CameraDolly.h
    rendering_interaction/CameraDolly.cpp
    rendering_interaction/InteractorStyleImage.h
    rendering_interaction/InteractorStyleImage.cpp
    rendering_interaction/InteractorStyleSwitch.h
    rendering_interaction/InteractorStyleSwitch.cpp
    rendering_interaction/InteractorStyleTerrain.h
    rendering_interaction/InteractorStyleTerrain.cpp
    rendering_interaction/Highlighter.h
    rendering_interaction/Highlighter.cpp
    rendering_interaction/ICameraInteractionStyle.h
    rendering_interaction/Picker.h
    rendering_interaction/Picker.cpp
    rendering_interaction/PickerHighlighterInteractorObserver.h
    rendering_interaction/PickerHighlighterInteractorObserver.cpp
    rendering_interaction/CameraInteractorStyleSwitch.h
    rendering_interaction/CameraInteractorStyleSwitch.cpp
    rendering_interaction/RenderWindowCursorCallback.h
    rendering_interaction/RenderWindowCursorCallback.cpp

    visualization_config/ColorMappingChooser.h
    visualization_config/ColorMappingChooser.cpp
    visualization_config/GlyphMappingChooser.h
    visualization_config/GlyphMappingChooser.cpp
    visualization_config/GlyphMappingChooserListModel.h
    visualization_config/GlyphMappingChooserListModel.cpp
    visualization_config/RendererConfigWidget.h
    visualization_config/RendererConfigWidget.cpp
    visualization_config/RendererConfigWidget_Plot.cpp
    visualization_config/RendererConfigWidget_Renderer.cpp
    visualization_config/RendererConfigWidget_Renderer2.cpp
    visualization_config/RenderPropertyConfigWidget.h
    visualization_config/RenderPropertyConfigWidget.cpp
    visualization_config/ResidualViewConfigWidget.h
    visualization_config/ResidualViewConfigWidget.cpp

    widgets/CollapsibleGroupBox.h
    widgets/CollapsibleGroupBox.cpp
    widgets/DockableWidget.h
    widgets/DockableWidget.cpp
    widgets/DoubleSpinBox.h
    widgets/DoubleSpinBox.cpp
)

set(UIs
    MainWindow.ui

    data_view/TableView.ui

    data_handling/CoordinateSystemAdjustmentWidget.ui
    data_handling/DataBrowser.ui
    data_handling/DEMWidget.ui

    io/CanvasExporterWidget.ui
    io/DataImporterWidget.ui
    io/GridDataImporterWidget.ui

    visualization_config/ColorMappingChooser.ui
    visualization_config/GlyphMappingChooser.ui
    visualization_config/RendererConfigWidget.ui
    visualization_config/RenderPropertyConfigWidget.ui
    visualization_config/ResidualViewConfigWidget.ui
)

set(QRCs
    icons.qrc
)

qt5_wrap_ui(UI_Srcs ${UIs})
qt5_add_resources(QRC_Srcs ${QRCs})


# Group source files
source_group_by_path_and_type(${CMAKE_CURRENT_SOURCE_DIR} ${sources} ${UIs} ${QRCs})
source_group_by_path(${CMAKE_CURRENT_BINARY_DIR} ".*" "Generated" ${CMAKE_CURRENT_BINARY_DIR}/${target}_api.h ${UI_Srcs} ${QRC_Srcs})
source_group_by_path(${CMAKE_CURRENT_SOURCE_DIR} ".*" "" "CMakeLists.txt")

add_library(${target} SHARED ${sources} ${UI_Srcs} ${QRC_Srcs})

target_include_directories(${target}
    PUBLIC
        ${PROJECT_SOURCE_DIR}
        ${PROJECT_BINARY_DIR}
    PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}
)

target_include_directories(${target} SYSTEM
    PUBLIC
        ${Qt5Gui_INCLUDE_DIRS} # disable warnings produced by Qt headers (GCC)
)

target_link_libraries(${target}
    PUBLIC
        Qt5::Widgets
        Qt5::OpenGL
        Qt5::Concurrent
        core
)

configure_cxx_target(${target})

generate_library_export_header(${target})


install(TARGETS ${target}
    RUNTIME DESTINATION ${INSTALL_BIN}
    LIBRARY DESTINATION ${INSTALL_SHARED}
    # ARCHIVE DESTINATION ${INSTALL_LIB}
)


add_subdirectory(widgets/qt_designer_plugins)
