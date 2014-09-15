#pragma once

#include <core/vector_mapping/VectorsForSurfaceMapping.h>


class vtkDataArray;
class vtkFloatArray;

class AttributeVectorData;


class AttributeVectorMapping : public VectorsForSurfaceMapping
{
public:
    ~AttributeVectorMapping() override;

    QString name() const override;

    vtkIdType maximumStartingIndex() override;

protected:
    static QList<VectorsForSurfaceMapping *> newInstances(RenderedData * renderedData);

    AttributeVectorMapping(RenderedData * renderedData, AttributeVectorData * attributeVector);

    void initialize() override;

    void startingIndexChangedEvent() override;

private:
    static const bool s_registered;

    AttributeVectorData * m_attributeVector;

    vtkSmartPointer<vtkFloatArray> m_sectionArray;
    vtkSmartPointer<vtkPolyData> m_processedPoints;
};
