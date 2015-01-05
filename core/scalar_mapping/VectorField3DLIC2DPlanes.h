#pragma once

#include <core/scalar_mapping/ScalarsForColorMapping.h>


class RenderedVectorGrid3D;


class CORE_API VectorField3DLIC2DPlanes : public ScalarsForColorMapping
{
public:
    VectorField3DLIC2DPlanes(const QList<AbstractVisualizedData *> & visualizedData);
    ~VectorField3DLIC2DPlanes() override;

    QString name() const override;

    void configureMapper(AbstractVisualizedData * visualizedData, vtkMapper * mapper) override;

protected:
    void updateBounds() override;

private:
    QList<RenderedVectorGrid3D *> m_vectorGrids;

    static const QString s_name;
    static const bool s_isRegistered;
};
