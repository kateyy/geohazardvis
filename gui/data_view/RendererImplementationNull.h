#pragma once

#include <vtkSmartPointer.h>

#include <gui/data_view/RendererImplementation.h>


class vtkRenderer;


class RendererImplementationNull : public RendererImplementation
{
public:
    RendererImplementationNull(AbstractRenderView & renderView);

    QString name() const override;
    ContentType contentType() const override;

    QList<DataObject *> filterCompatibleObjects(const QList<DataObject *> & dataObjects, QList<DataObject *> & incompatibleObjects);

    void activate(QVTKWidget & qvtkWidget) override;
    void deactivate(QVTKWidget & qvtkWidget) override;

    void render() override;
    vtkRenderWindowInteractor * interactor() override;

    void setSelectedData(AbstractVisualizedData * vis, vtkIdType index, IndexType indexType) override;
    void setSelectedData(AbstractVisualizedData * vis, vtkIdTypeArray & indices, IndexType indexType) override;
    void clearSelection() override;
    AbstractVisualizedData * selectedData() const override;
    vtkIdType selectedIndex() const override;
    IndexType selectedIndexType() const override;
    void lookAtData(AbstractVisualizedData & vis, vtkIdType index, IndexType indexType, unsigned int subViewIndex) override;
    void resetCamera(bool toInitialPosition, unsigned int subViewIndex) override;

    void setAxesVisibility(bool) override;
    bool canApplyTo(const QList<DataObject *> &) override;

protected:
    std::unique_ptr<AbstractVisualizedData> requestVisualization(DataObject &) const override;
    void onAddContent(AbstractVisualizedData *, unsigned int) override;
    void onRemoveContent(AbstractVisualizedData *, unsigned int) override;

private:
    vtkSmartPointer<vtkRenderer> m_renderer;
};
