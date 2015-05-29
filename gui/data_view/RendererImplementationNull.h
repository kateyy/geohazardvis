#pragma once

#include <vtkSmartPointer.h>

#include <gui/data_view/RendererImplementation.h>


class vtkRenderer;


class RendererImplementationNull : public RendererImplementation
{
public:
    RendererImplementationNull(AbstractRenderView & renderView, QObject * parent = nullptr);

    QString name() const override;
    ContentType contentType() const override;

    QList<DataObject *> filterCompatibleObjects(const QList<DataObject *> & dataObjects, QList<DataObject *> & incompatibleObjects);

    void activate(QVTKWidget * qvtkWidget) override;
    void deactivate(QVTKWidget * qvtkWidget) override;

    void render() override;
    vtkRenderWindowInteractor * interactor() override;
    void setSelectedData(DataObject *, vtkIdType = -1) override;
    DataObject * selectedData() const override;
    vtkIdType selectedIndex() const override;
    void lookAtData(DataObject *, vtkIdType) override;
    void resetCamera(bool) override;
    void setAxesVisibility(bool) override;
    bool canApplyTo(const QList<DataObject *> &) override;

protected:
    AbstractVisualizedData * requestVisualization(DataObject *) const override;
    void onAddContent(AbstractVisualizedData *, unsigned int) override;
    void onRemoveContent(AbstractVisualizedData *, unsigned int) override;

private:
    vtkSmartPointer<vtkRenderer> m_renderer;
};
