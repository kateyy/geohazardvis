#pragma once

#include <vtkSmartPointer.h>

#include <core/vector_mapping/VectorsForSurfaceMapping.h>


class vtkDataArray;


class CORE_API AttributeVectorMapping : public VectorsForSurfaceMapping
{
public:
    ~AttributeVectorMapping() override;

    QString name() const override;

protected:
    /** create an instance for each 3D vector array found in the renderedData */
    static QList<VectorsForSurfaceMapping *> newInstances(RenderedData * renderedData);

    /** create an instances that maps vectors from vectorData to the renderedData's geometry */
    AttributeVectorMapping(RenderedData * renderedData, vtkDataArray * vectorData);

    void initialize() override;

private:
    static const bool s_registered;

    vtkSmartPointer<vtkDataArray> m_dataArray;
};
