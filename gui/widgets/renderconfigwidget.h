#pragma once

#include <QDockWidget>
#include <QVector>

class QImage;
class vtkProperty;
namespace reflectionzeug {
    class PropertyGroup;
}
namespace propertyguizeug {
    class PropertyBrowser;
}
class Ui_RenderConfigWidget;

class RenderConfigWidget : public QDockWidget
{
    Q_OBJECT

public:
    RenderConfigWidget(QWidget * parent = nullptr);
    ~RenderConfigWidget() override;

    void setRenderProperty(vtkProperty * property);
    void clear();

    const QImage & selectedGradient() const;

signals:
    void configChanged();

protected:
    void loadGradientImages();
    void updatePropertyBrowser();

protected:
    Ui_RenderConfigWidget * m_ui;
    reflectionzeug::PropertyGroup * m_propertyRoot;
    propertyguizeug::PropertyBrowser * m_propertyBrowser;
    vtkProperty * m_renderProperty;

    QVector<QImage> m_scalarToColorGradients;
};
