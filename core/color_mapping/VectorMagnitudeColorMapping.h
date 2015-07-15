#pragma once

#include <QMap>

#include <core/color_mapping/ColorMappingData.h>


template<typename T> class QVector;

class vtkVectorNorm;


class CORE_API VectorMagnitudeColorMapping : public ColorMappingData
{
public:
    VectorMagnitudeColorMapping(
        const QList<AbstractVisualizedData *> & visualizedData,
        const QString & dataArrayName, int attributeLocation);
    ~VectorMagnitudeColorMapping() override;

    QString name() const override;

    vtkSmartPointer<vtkAlgorithm> createFilter(AbstractVisualizedData * visualizedData, int connection = 0) override;
    bool usesFilter() const override;

    void configureMapper(AbstractVisualizedData * visualizedData, vtkAbstractMapper * mapper) override;

protected:
    static std::vector<std::unique_ptr<ColorMappingData>> newInstances(const QList<AbstractVisualizedData*> & visualizedData);

    QMap<int, QPair<double, double>> updateBounds() override;

private:
    static const bool s_isRegistered;

    const int m_attributeLocation;
    const QString m_dataArrayName;

    QMap<AbstractVisualizedData *, QVector<vtkSmartPointer<vtkVectorNorm>>> m_vectorNorms;
};
