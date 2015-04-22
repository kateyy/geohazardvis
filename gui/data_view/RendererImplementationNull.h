#pragma once

#include <gui/data_view/RendererImplementation.h>


class RendererImplementationNull : public RendererImplementation
{
public:
    RendererImplementationNull(AbstractRenderView & renderView, QObject * parent = nullptr);

    QString name() const override { return {}; }
    ContentType contentType() const override;

    QList<DataObject *> filterCompatibleObjects(const QList<DataObject *> & dataObjects, QList<DataObject *> & incompatibleObjects)
    {
        incompatibleObjects = dataObjects;
        return{};
    }

    void render() override {}
    vtkRenderWindowInteractor * interactor() override { return nullptr; }
    void setSelectedData(DataObject *, vtkIdType = -1) override { }
    DataObject * selectedData() const override { return nullptr; }
    vtkIdType selectedIndex() const override { return -1; }
    void lookAtData(DataObject *, vtkIdType) override { }
    void resetCamera(bool) override { }
    void setAxesVisibility(bool) override { }
    bool canApplyTo(const QList<DataObject *> &) override { return false; }

protected:
    AbstractVisualizedData * requestVisualization(DataObject *) const override { return nullptr; }
    void onAddContent(AbstractVisualizedData *, unsigned int) override { }
    void onRemoveContent(AbstractVisualizedData *, unsigned int) override
    {
    }
};
