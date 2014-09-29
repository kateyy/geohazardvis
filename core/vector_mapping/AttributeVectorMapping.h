#pragma once

#include <core/vector_mapping/VectorsForSurfaceMapping.h>


class vtkFloatArray;
class vtkEventQtSlotConnect;

class AttributeVectorData;


class AttributeVectorMapping : public VectorsForSurfaceMapping
{
    Q_OBJECT

public:
    ~AttributeVectorMapping() override;

    QString name() const override;

    vtkIdType maximumStartingIndex() override;

protected:
    static QList<VectorsForSurfaceMapping *> newInstances(RenderedData * renderedData);

    AttributeVectorMapping(RenderedData * renderedData, AttributeVectorData * attributeVector);

    void initialize() override;

    void startingIndexChangedEvent() override;

protected slots:
    void updateForChangedData();

private:
    static const bool s_registered;

    AttributeVectorData * m_attributeVector;

    vtkSmartPointer<vtkFloatArray> m_sectionArray;

    vtkSmartPointer<vtkEventQtSlotConnect> m_vtkQtConnect;
};
