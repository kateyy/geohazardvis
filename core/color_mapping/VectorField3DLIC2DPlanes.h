#pragma once

#include <QVector>

#include <core/color_mapping/ColorMappingData.h>


class vtkImageDataLIC2D;
class vtkRenderWindow;

class NoiseImageSource;
class RenderedVectorGrid3D;


class CORE_API VectorField3DLIC2DPlanes : public ColorMappingData
{
public:
    VectorField3DLIC2DPlanes(const QList<AbstractVisualizedData *> & visualizedData);
    ~VectorField3DLIC2DPlanes() override;

    QString name() const override;

    vtkAlgorithm * createFilter(AbstractVisualizedData * visualizedData, int connection) override;
    bool usesFilter() const override;


protected:
    QMap<vtkIdType, QPair<double, double>> updateBounds() override;

    vtkRenderWindow * glContext();

private:
    QList<RenderedVectorGrid3D *> m_vectorGrids;

    vtkSmartPointer<NoiseImageSource> m_noiseImage;
    QMap<AbstractVisualizedData *, QVector<vtkSmartPointer<vtkImageDataLIC2D>>> m_lic2D;

    vtkSmartPointer<vtkRenderWindow> m_glContext;

    static const QString s_name;
    static const bool s_isRegistered;
};
