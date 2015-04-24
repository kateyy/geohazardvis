#pragma once

#include <QDockWidget>
#include <QMap>

#include <vtkSmartPointer.h>


class vtkObject;
class vtkCollection;
class vtkEventQtSlotConnect;
namespace reflectionzeug
{
    class PropertyGroup;
}
class Ui_RendererConfigWidget;
class AbstractRenderView;
class RendererImplementationBase3D;
class RendererImplementationPlot;


class RendererConfigWidget : public QDockWidget
{
    Q_OBJECT

public:
    RendererConfigWidget(QWidget * parent = nullptr);
    ~RendererConfigWidget() override;

    void clear();

public slots:
    void setRenderViews(const QList<AbstractRenderView *> & renderViews);
    void setCurrentRenderView(AbstractRenderView * renderView);

private slots:
    void setCurrentRenderView(int index);
    void updateRenderViewTitle(const QString & newTitle);
    void readCameraStats(vtkObject * caller);

private:
    reflectionzeug::PropertyGroup * createPropertyGroup(AbstractRenderView * renderView);
    reflectionzeug::PropertyGroup * createPropertyGroupRenderer(
        AbstractRenderView * renderView, RendererImplementationBase3D * impl);
    reflectionzeug::PropertyGroup * createPropertyGroupPlot(
        AbstractRenderView * renderView, RendererImplementationPlot * impl);

private:
    Ui_RendererConfigWidget * m_ui;

    reflectionzeug::PropertyGroup * m_propertyRoot;
    AbstractRenderView * m_currentRenderView;
    vtkSmartPointer<vtkEventQtSlotConnect> m_eventConnect;
    /** vtkEventQtSlotConnect does not increase the reference count of signal sources, so we do this manually */
    vtkSmartPointer<vtkCollection> m_eventEmitters;
};
