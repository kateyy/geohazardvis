#pragma once

#include <QDockWidget>


namespace reflectionzeug {
    class PropertyGroup;
}
class Ui_RenderConfigWidget;
class RenderedData;


class RenderConfigWidget : public QDockWidget
{
    Q_OBJECT

public:
    RenderConfigWidget(QWidget * parent = nullptr);
    ~RenderConfigWidget() override;

    /** reset the property browser to contain only configuration options related to the rendered data */
    void setRenderedData(RenderedData * renderedData);

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
};
