#pragma once

#include <QMap>

#include <vtkSmartPointer.h>

#include <core/scalar_mapping/ScalarsForColorMapping.h>


class vtkVectorNorm;


class CORE_API VectorMagnitudeMapping : public ScalarsForColorMapping
{
public:
    VectorMagnitudeMapping(
        const QList<AbstractVisualizedData *> & visualizedData,
        const QString & dataArrayName, int attributeLocation);
    ~VectorMagnitudeMapping() override;

    QString name() const override;

    vtkAlgorithm * createFilter(AbstractVisualizedData * visualizedData) override;
    bool usesFilter() const override;

    void configureMapper(AbstractVisualizedData * visualizedData, vtkMapper * mapper) override;

protected:
    static QList<ScalarsForColorMapping *> newInstances(const QList<AbstractVisualizedData*> & visualizedData);

    void updateBounds() override;

private:
    static const bool s_isRegistered;

    const int m_attributeLocation;
    const QString m_dataArrayName;

    QMap<AbstractVisualizedData *, vtkSmartPointer<vtkVectorNorm>> m_vectorNorms;
};
