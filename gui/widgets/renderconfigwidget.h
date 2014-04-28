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
class InputViewer;

class RenderConfigWidget : public QDockWidget
{
    Q_OBJECT

public:
    RenderConfigWidget(QWidget * parent = nullptr);
    ~RenderConfigWidget() override;

    void setViewer(InputViewer * viewer);
    void setRenderProperty(vtkProperty * property);
    void clear();

    const QImage & selectedGradient() const;

signals:
    void gradientSelectionChanged(const QImage & currentGradient);

protected slots:
    void updateGradientSelection(int selection);

protected:
    void paintEvent(QPaintEvent * event) override;

    void loadGradientImages();
    void updatePropertyBrowser();

protected:
    Ui_RenderConfigWidget * m_ui;
    bool m_needsBrowserRebuild;

    reflectionzeug::PropertyGroup * m_propertyRoot;
    propertyguizeug::PropertyBrowser * m_propertyBrowser;

    InputViewer * m_viewer;
    vtkProperty * m_renderProperty;

    QVector<QImage> m_scalarToColorGradients;
};
