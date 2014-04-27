#pragma once

#include <QDockWidget>

class vtkProperty;
namespace reflectionzeug {
    class PropertyGroup;
}
namespace propertyguizeug {
    class PropertyBrowser;
}

class RenderConfigWidget : public QDockWidget
{
    Q_OBJECT

public:
    RenderConfigWidget(QWidget * parent = nullptr);
    ~RenderConfigWidget() override;

    void setRenderProperty(vtkProperty * property);
    void clear();

signals:
    void configChanged();

protected:
    virtual void updatePropertyBrowser();

protected:
    reflectionzeug::PropertyGroup * m_propertyRoot;
    propertyguizeug::PropertyBrowser * m_propertyBrowser;
    vtkProperty * m_renderProperty;
};
