
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

    widgets/CanvasExporterWidget.h
    widgets/CanvasExporterWidget.cpp
    widgets/ColorMappingChooser.h
    widgets/ColorMappingChooser.cpp
    widgets/CollapsibleGroupBox.h
    widgets/CollapsibleGroupBox.cpp
    widgets/CoordinateSystemAdjustmentWidget.h
    widgets/CoordinateSystemAdjustmentWidget.cpp
    widgets/DataBrowser.h
    widgets/DataBrowser.cpp
    widgets/DataBrowserTableModel.h
    widgets/DataBrowserTableModel.cpp
    widgets/DataImporterWidget.h
    widgets/DataImporterWidget.cpp
    widgets/DEMWidget.h
    widgets/DEMWidget.cpp
    widgets/DockableWidget.h
    widgets/DockableWidget.cpp
    widgets/GlyphMappingChooser.h
    widgets/GlyphMappingChooser.cpp
    widgets/GlyphMappingChooserListModel.h
    widgets/GlyphMappingChooserListModel.cpp
    widgets/GridDataImporterWidget.h
    widgets/GridDataImporterWidget.cpp
    widgets/RendererConfigWidget.h
    widgets/RendererConfigWidget.cpp
    widgets/RendererConfigWidget_Plot.cpp
    widgets/RendererConfigWidget_Renderer.cpp
    widgets/RendererConfigWidget_Renderer2.cpp
    widgets/RenderPropertyConfigWidget.h
    widgets/RenderPropertyConfigWidget.cpp
    widgets/ResidualViewConfigWidget.h
    widgets/ResidualViewConfigWidget.cpp
)

set(UIs
    MainWindow.ui

    data_view/TableView.ui

    widgets/CanvasExporterWidget.ui
    widgets/ColorMappingChooser.ui
    widgets/CoordinateSystemAdjustmentWidget.ui
    widgets/DataBrowser.ui
    widgets/DataImporterWidget.ui
    widgets/DEMWidget.ui
    widgets/GlyphMappingChooser.ui
    widgets/GridDataImporterWidget.ui
    widgets/RendererConfigWidget.ui
    widgets/RenderPropertyConfigWidget.ui
    widgets/ResidualViewConfigWidget.ui
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
    PRIVATE ${CMAKE_CURRENT_BINARY_DIR}
)

target_include_directories(${target} SYSTEM
    PUBLIC ${Qt5Gui_INCLUDE_DIRS} # disable warnings produced by Qt headers (GCC)
)

target_link_libraries(${target}
    PUBLIC
        Qt5::Widgets
        Qt5::OpenGL
        core
)

if (NOT ${Qt5Core_VERSION} VERSION_LESS 5.5)
    target_link_libraries(${target} PUBLIC Qt5::Concurrent)
endif()


configure_cxx_target(${target})

generate_library_export_header(${target})


install(TARGETS ${target}
    RUNTIME DESTINATION ${INSTALL_BIN}
    LIBRARY DESTINATION ${INSTALL_SHARED}
    # ARCHIVE DESTINATION ${INSTALL_LIB}
)
