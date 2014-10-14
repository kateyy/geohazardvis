#pragma once

#include <vtkSmartPointer.h>

#include <core/vector_mapping/VectorMappingData.h>


class vtkDataArray;

class PolyDataObject;


class CORE_API PolyDataAttributeVectorMapping : public VectorMappingData
{
public:
    ~PolyDataAttributeVectorMapping() override;

    QString name() const override;

protected:
    /** create an instance for each 3D vector array found in the renderedData */
    static QList<VectorMappingData *> newInstances(RenderedData * renderedData);

    /** create an instances that maps vectors from vectorData to the renderedData's geometry */
    PolyDataAttributeVectorMapping(RenderedData * renderedData, vtkDataArray * vectorData);

    void initialize() override;

private:
    static const bool s_registered;

    PolyDataObject * m_polyData;
    vtkSmartPointer<vtkDataArray> m_dataArray;
};
