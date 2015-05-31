#pragma once

#include <QDockWidget>

#include <gui/gui_api.h>


namespace reflectionzeug
{
    class PropertyGroup;
}
class Ui_RenderConfigWidget;
class AbstractRenderView;
class AbstractVisualizedData;
class DataObject;


class GUI_API RenderConfigWidget : public QDockWidget
{
    Q_OBJECT

public:
    RenderConfigWidget(QWidget * parent = nullptr);
    ~RenderConfigWidget() override;

public slots:
    void setCurrentRenderView(AbstractRenderView * renderView = nullptr);
    /** switch to specified dataObject, in case it is visible in my current render view */
    void setSelectedData(DataObject * dataObject);
    void setSelectedData(AbstractRenderView * renderView, DataObject * dataObject);
    void clear();

private slots:
    /** remove data from the UI if we currently hold it */
    void checkDeletedContent(AbstractVisualizedData * content);

protected:
    void updateTitle();

protected:
    Ui_RenderConfigWidget * m_ui;

    reflectionzeug::PropertyGroup * m_propertyRoot;

    AbstractRenderView * m_renderView;
    AbstractVisualizedData * m_content;
};
