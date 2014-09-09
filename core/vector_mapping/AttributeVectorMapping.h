#pragma once

#include <core/vector_mapping/VectorsForSurfaceMapping.h>


class vtkDataArray;

class AttributeVectorData;


class AttributeVectorMapping : public VectorsForSurfaceMapping
{
public:
    ~AttributeVectorMapping() override;

    QString name() const override;

protected:
    static QList<VectorsForSurfaceMapping *> newInstances(RenderedData * renderedData);

    AttributeVectorMapping(RenderedData * renderedData, AttributeVectorData * attributeVector);

    void initialize() override;

private:
    static const bool s_registered;

    AttributeVectorData * m_attributeVector;
};
