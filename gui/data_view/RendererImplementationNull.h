#pragma once

#include <vtkSmartPointer.h>

#include <gui/data_view/RendererImplementation.h>


class vtkRenderer;


class GUI_API RendererImplementationNull : public RendererImplementation
{
public:
    explicit RendererImplementationNull(AbstractRenderView & renderView);
    ~RendererImplementationNull() override;

    QString name() const override;
    ContentType contentType() const override;

    QList<DataObject *> filterCompatibleObjects(const QList<DataObject *> & dataObjects, QList<DataObject *> & incompatibleObjects);

    void activate(t_QVTKWidget & qvtkWidget) override;
    void deactivate(t_QVTKWidget & qvtkWidget) override;

    void render() override;
    vtkRenderWindowInteractor * interactor() override;

    void lookAtData(const VisualizationSelection & selection, unsigned int subViewIndex) override;
    void resetCamera(bool toInitialPosition, unsigned int subViewIndex) override;

    void setAxesVisibility(bool) override;
    bool canApplyTo(const QList<DataObject *> &) override;

protected:
    std::unique_ptr<AbstractVisualizedData> requestVisualization(DataObject &) const override;
    void onAddContent(AbstractVisualizedData *, unsigned int) override;
    void onRemoveContent(AbstractVisualizedData *, unsigned int) override;

    void onSetSelection(const VisualizationSelection & selection) override;
    void onClearSelection() override;

private:
    vtkSmartPointer<vtkRenderer> m_renderer;

private:
    Q_DISABLE_COPY(RendererImplementationNull)
};
