#pragma once

#include <memory>

#include <QDockWidget>
#include <QMap>

#include <vtkSmartPointer.h>

#include <gui/gui_api.h>


class vtkCamera;
class vtkObject;
namespace reflectionzeug
{
    class PropertyGroup;
}
class Ui_RendererConfigWidget;
class AbstractRenderView;
class CollapsibleGroupBox;
class RendererImplementationBase3D;
class RendererImplementationPlot;
class ResidualViewConfigWidget;


class GUI_API RendererConfigWidget : public QDockWidget
{
public:
    explicit RendererConfigWidget(QWidget * parent = nullptr);
    ~RendererConfigWidget() override;

    void clear();

public:
    void setCurrentRenderView(AbstractRenderView * renderView);

private:
    void readCameraStats(vtkObject * caller, unsigned long, void *);

    void updateTitle();
    void updateForNewImplementation();
    void updateInteractionModeCombo();
    /** Adjust visible properties for the current configuration.
      * This currently requires to recreate all properties. */
    void updatePropertyGroup();

    void setInteractionStyle(const QString & styleName);

    reflectionzeug::PropertyGroup * createPropertyGroup(AbstractRenderView * renderView);
    reflectionzeug::PropertyGroup * createPropertyGroupRenderer(
        AbstractRenderView * renderView, RendererImplementationBase3D * impl);
    reflectionzeug::PropertyGroup * createPropertyGroupPlot(
        AbstractRenderView * renderView, RendererImplementationPlot * impl);

private:
    std::unique_ptr<Ui_RendererConfigWidget> m_ui;
    ResidualViewConfigWidget * m_residualUi;
    CollapsibleGroupBox * m_residualGroupBox;

    reflectionzeug::PropertyGroup * m_propertyRoot;
    AbstractRenderView * m_currentRenderView;
    QMap<vtkSmartPointer<vtkCamera>, unsigned long> m_cameraObserverTags;
};
