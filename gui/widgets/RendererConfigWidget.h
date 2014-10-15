#pragma once

#include <QDockWidget>
#include <QMap>

#include <vtkSmartPointer.h>


class vtkObject;
class vtkCamera;
class vtkEventQtSlotConnect;
namespace reflectionzeug
{
    class PropertyGroup;
}
namespace propertyguizeug
{
    class PropertyBrowser;
}
class Ui_RendererConfigWidget;
class RenderView;


class RendererConfigWidget : public QDockWidget
{
    Q_OBJECT

public:
    RendererConfigWidget(QWidget * parent = nullptr);
    ~RendererConfigWidget() override;

    void clear();

public slots:
    void setRenderViews(const QList<RenderView *> & renderViews);
    void setCurrentRenderView(RenderView * renderView);

private slots:
    void setCurrentRenderView(int index);
    void updateRenderViewTitle(const QString & newTitle);
    void readCameraStats(vtkObject * caller);

private:
    reflectionzeug::PropertyGroup * createPropertyGroup(RenderView * renderView);

private:
    Ui_RendererConfigWidget * m_ui;
    propertyguizeug::PropertyBrowser * m_propertyBrowser;

    reflectionzeug::PropertyGroup * m_propertyRoot;

    vtkCamera * m_activeCamera;

    vtkSmartPointer<vtkEventQtSlotConnect> m_eventConnect;
};
