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
        const QString & dataArrayName, IndexType attributeLocation);
    ~VectorMagnitudeColorMapping() override;

    QString name() const override;
    QString scalarsName(AbstractVisualizedData & vis) const override;
    IndexType scalarsAssociation(AbstractVisualizedData & vis) const override;

    vtkSmartPointer<vtkAlgorithm> createFilter(AbstractVisualizedData & visualizedData, int connection = 0) override;
    bool usesFilter() const override;

    void configureMapper(AbstractVisualizedData & visualizedData, vtkAbstractMapper & mapper) override;

protected:
    static std::vector<std::unique_ptr<ColorMappingData>> newInstances(const QList<AbstractVisualizedData*> & visualizedData);

    std::vector<ValueRange<>> updateBounds() override;

private:
    static const bool s_isRegistered;

    const IndexType m_attributeLocation;
    const QString m_dataArrayName;
    const QString m_magnitudeArrayName;

    QMap<AbstractVisualizedData *, QVector<vtkSmartPointer<vtkAlgorithm>>> m_filters;
};
