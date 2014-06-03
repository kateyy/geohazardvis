#pragma once

#include <QDockWidget>
#include <QVector>


class QImage;
class vtkProperty;
namespace reflectionzeug {
    class PropertyGroup;
}
class Ui_RenderConfigWidget;


class RenderConfigWidget : public QDockWidget
{
    Q_OBJECT

public:
    RenderConfigWidget(QWidget * parent = nullptr);
    ~RenderConfigWidget() override;

    void setRenderProperty(QString propertyName, vtkProperty * renderProperty);

    void addPropertyGroup(reflectionzeug::PropertyGroup * group);

    void clear();

signals:
    void renderPropertyChanged();

protected:
    void paintEvent(QPaintEvent * event) override;

    void updateWindowTitle(QString propertyName = "");

    void updatePropertyBrowser();

protected:
    Ui_RenderConfigWidget * m_ui;
    bool m_needsBrowserRebuild;

    reflectionzeug::PropertyGroup * m_propertyRoot;

    QVector<reflectionzeug::PropertyGroup *> m_addedGroups;

    vtkProperty * m_renderProperty;
};
