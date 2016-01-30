#pragma once

#include <memory>

#include <QDockWidget>
#include <QScopedPointer>

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
public:
    explicit RenderConfigWidget(QWidget * parent = nullptr);
    ~RenderConfigWidget() override;

public:
    void setCurrentRenderView(AbstractRenderView * renderView = nullptr);
    /** switch to specified dataObject, in case it is visible in my current render view */
    void setSelectedData(DataObject * dataObject);
    void setSelectedData(AbstractRenderView * renderView, DataObject * dataObject);
    void clear();

private:
    /** remove data from the UI if we currently hold it */
    void checkDeletedContent(const QList<AbstractVisualizedData *> & content);

    void updateTitle();

private:
    QScopedPointer<Ui_RenderConfigWidget> m_ui;

    std::unique_ptr<reflectionzeug::PropertyGroup> m_propertyRoot;

    AbstractRenderView * m_renderView;
    AbstractVisualizedData * m_content;
};
