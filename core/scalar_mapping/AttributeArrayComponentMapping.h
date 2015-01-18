#pragma once

#include <vtkType.h>

#include <core/scalar_mapping/ScalarsForColorMapping.h>


class CORE_API AttributeArrayComponentMapping : public ScalarsForColorMapping
{
public:
    AttributeArrayComponentMapping(const QList<AbstractVisualizedData *> & visualizedData,
        QString dataArrayName, int attributeLocation, vtkIdType numDataComponents);
    ~AttributeArrayComponentMapping() override;

    QString name() const override;
    QString scalarsName() const override;

    vtkAlgorithm * createFilter(AbstractVisualizedData * visualizedData) override;
    bool usesFilter() const override;

    void configureMapper(AbstractVisualizedData * visualizedData, vtkMapper * mapper) override;

protected:
    static QList<ScalarsForColorMapping *> newInstances(const QList<AbstractVisualizedData *> & visualizedData);

    void updateBounds() override;

private:
    static const bool s_isRegistered;

    const int m_attributeLocation;
    const QString m_dataArrayName;
};
