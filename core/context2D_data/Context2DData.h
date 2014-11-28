#pragma once

#include <vtkSmartPointer.h>

#include <core/AbstractVisualizedData.h>


class vtkContextItemCollection;


class CORE_API Context2DData : public AbstractVisualizedData
{
    Q_OBJECT

public:
    Context2DData(DataObject * dataObject);

    vtkSmartPointer<vtkContextItemCollection> contextItems();

protected:
    virtual vtkSmartPointer<vtkContextItemCollection> fetchContextItems() = 0;
    void invalidateContextItems();

signals:
    void contextItemCollectionChanged();

private:
    vtkSmartPointer<vtkContextItemCollection> m_viewProps;
    bool m_contextItemsInvalid;
};
