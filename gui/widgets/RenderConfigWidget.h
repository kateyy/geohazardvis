#pragma once

#include <memory>

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
class TemporalPipelineMediator;
struct VisualizationSelection;


class GUI_API RenderConfigWidget : public QDockWidget
{
public:
    explicit RenderConfigWidget(QWidget * parent = nullptr);
    ~RenderConfigWidget() override;

public:
    void setCurrentRenderView(AbstractRenderView * renderView = nullptr);
    void setSelectedData(DataObject * dataObject);
    void setSelectedVisualization(AbstractRenderView * renderView, const VisualizationSelection & selection);
    void clear();

    void setCurrentTimeStep(int timeStepIndex);

private:
    /** remove data from the UI if we currently hold it */
    void checkDeletedContent(const QList<AbstractVisualizedData *> & content);

    void updateTitle();

private:
    std::unique_ptr<Ui_RenderConfigWidget> m_ui;
    std::unique_ptr<TemporalPipelineMediator> m_temporalSelector;

    std::unique_ptr<reflectionzeug::PropertyGroup> m_propertyRoot;

    AbstractRenderView * m_renderView;
    AbstractVisualizedData * m_content;

private:
    Q_DISABLE_COPY(RenderConfigWidget)
};
