#pragma once

#include <vtkSmartPointer.h>

#include <core/vector_mapping/VectorMappingData.h>


class vtkDataArray;
class RenderedVectorGrid3D;


class CORE_API Grid3dVectorMapping : public VectorMappingData
{
    Q_OBJECT

public:
    QString name() const override;

protected:
    /** create an instance for each 3D vector array found in the renderedData */
    static QList<VectorMappingData *> newInstances(RenderedData * renderedData);

    Grid3dVectorMapping(RenderedVectorGrid3D * renderedGrid, vtkDataArray * dataArray);

    void initialize() override;

protected slots:
    void updateArrowLength();

private:
    static const bool s_registered;

    RenderedVectorGrid3D * m_renderedGrid;
    vtkDataArray * m_dataArray;
};
