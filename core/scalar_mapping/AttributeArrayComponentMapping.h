#pragma once

#include <vtkType.h>

#include <core/scalar_mapping/ScalarsForColorMapping.h>


class CORE_API AttributeArrayComponentMapping : public ScalarsForColorMapping
{
public:
    AttributeArrayComponentMapping(const QList<DataObject *> & dataObjects, QString dataArrayName, int attributeLocation, vtkIdType numDataComponents);
    ~AttributeArrayComponentMapping() override;

    QString name() const override;
    QString scalarsName() const override;

    vtkAlgorithm * createFilter(DataObject * dataObject) override;
    bool usesFilter() const override;

    void configureDataObjectAndMapper(DataObject * dataObject, vtkMapper * mapper) override;

protected:
    static QList<ScalarsForColorMapping *> newInstances(const QList<DataObject *> & dataObjects);

    void updateBounds() override;

private:
    static const bool s_isRegistered;

    QString m_dataArrayName;

    const int m_attributeLocation;
};
