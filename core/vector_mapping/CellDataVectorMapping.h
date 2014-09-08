#pragma once

#include <vtkSmartPointer.h>

#include <core/vector_mapping/VectorsForSurfaceMapping.h>


class vtkDataArray;


class CORE_API CellDataVectorMapping : public VectorsForSurfaceMapping
{
    Q_OBJECT

public:
    ~CellDataVectorMapping() override;

    QString name() const override;

protected:
    /** create an instance for each 3D vector array found in the renderedData */
    static QList<VectorsForSurfaceMapping *> newInstances(RenderedData * renderedData);

    /** create an instances that maps vectors from vectorData to the renderedData's geometry */
    CellDataVectorMapping(RenderedData * renderedData, vtkDataArray * vectorData);

    void initialize() override;

private:
    static const bool s_registered;

    vtkSmartPointer<vtkDataArray> m_dataArray;
};
