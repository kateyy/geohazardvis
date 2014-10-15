#pragma once

#include <QDockWidget>


namespace reflectionzeug
{
    class PropertyGroup;
}
namespace propertyguizeug
{
    class PropertyBrowser;
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
    void setRenderedData(int rendererId = -1, RenderedData * renderedData = nullptr);
    int rendererId() const;
    RenderedData * renderedData();

    void clear();

protected:
    void updateTitle();

protected:
    Ui_RenderConfigWidget * m_ui;
    propertyguizeug::PropertyBrowser * m_propertyBrowser;

    reflectionzeug::PropertyGroup * m_propertyRoot;

    int m_rendererId;
    RenderedData * m_renderedData;
};
