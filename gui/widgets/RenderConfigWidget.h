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
class DataObject;
class RenderedData;
class RenderView;


class RenderConfigWidget : public QDockWidget
{
    Q_OBJECT

public:
    RenderConfigWidget(QWidget * parent = nullptr);
    ~RenderConfigWidget() override;

public slots:
    void setCurrentRenderView(RenderView * renderView = nullptr);
    /** switch to specified dataObject, in case it is visible in my current render view */
    void setSelectedData(DataObject * dataObject);
    void clear();

private slots:
    /** remove data from the UI if we currently hold it */
    void checkRemovedData(RenderedData * renderedData);

protected:
    void updateTitle();

protected:
    Ui_RenderConfigWidget * m_ui;
    propertyguizeug::PropertyBrowser * m_propertyBrowser;

    reflectionzeug::PropertyGroup * m_propertyRoot;

    RenderView * m_renderView;
    RenderedData * m_renderedData;
};
