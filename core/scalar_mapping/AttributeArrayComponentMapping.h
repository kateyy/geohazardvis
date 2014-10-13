#pragma once

#include <core/scalar_mapping/AbstractArrayComponentMapping.h>


class CORE_API AttributeArrayComponentMapping : public AbstractArrayComponentMapping
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
    static const bool s_registered;

    const int m_attributeLocation;
};
