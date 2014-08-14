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
    const RenderedData * renderedData() const;

    void clear();

protected:
    void updateWindowTitle(RenderedData * renderedData = nullptr);

protected:
    Ui_RenderConfigWidget * m_ui;

    reflectionzeug::PropertyGroup * m_propertyRoot;

    RenderedData * m_renderedData;
};
