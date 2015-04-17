#pragma once

#include <gui/data_view/RendererImplementation.h>


class RendererImplementationNull : public RendererImplementation
{
public:
    RendererImplementationNull(RenderView & renderView, QObject * parent = nullptr);

    QString name() const { return {}; }
    ContentType contentType() const;

    QList<DataObject *> filterCompatibleObjects(const QList<DataObject *> & dataObjects, QList<DataObject *> & incompatibleObjects)
    {
        incompatibleObjects = dataObjects;
        return{};
    }

    void render() {}
    vtkRenderWindowInteractor * interactor() { return nullptr; }
    void highlightData(DataObject *, vtkIdType = -1) { }
    DataObject * highlightedData() { return nullptr; }
    void lookAtData(DataObject *, vtkIdType) { }
    void resetCamera(bool) { }
    void setAxesVisibility(bool) { }
    bool canApplyTo(const QList<DataObject *> &) { return false; }

protected:
    AbstractVisualizedData * requestVisualization(DataObject *) const { return nullptr; }
    void onAddContent(AbstractVisualizedData *) { }
    void onRemoveContent(AbstractVisualizedData *) { }
};
