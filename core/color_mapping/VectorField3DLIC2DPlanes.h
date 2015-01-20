#pragma once

#include <core/color_mapping/ColorMappingData.h>


class RenderedVectorGrid3D;


class CORE_API VectorField3DLIC2DPlanes : public ColorMappingData
{
public:
    VectorField3DLIC2DPlanes(const QList<AbstractVisualizedData *> & visualizedData);
    ~VectorField3DLIC2DPlanes() override;

    QString name() const override;

    void configureMapper(AbstractVisualizedData * visualizedData, vtkMapper * mapper) override;

protected:
    QMap<vtkIdType, QPair<double, double>> updateBounds() override;

private:
    QList<RenderedVectorGrid3D *> m_vectorGrids;

    static const QString s_name;
    static const bool s_isRegistered;
};
