#pragma once

#include <QMap>

#include <core/scalar_mapping/AbstractArrayComponentMapping.h>


class vtkFloatArray;


class CORE_API RawArrayComponentMapping : public AbstractArrayComponentMapping
{
public:
    RawArrayComponentMapping(const QList<DataObject *> & dataObjects, vtkFloatArray * dataArray, vtkIdType component);
    ~RawArrayComponentMapping() override;

    vtkAlgorithm * createFilter(DataObject * dataObject) override;
    bool usesFilter() const override;

    void configureDataObjectAndMapper(DataObject * dataObject, vtkMapper * mapper) override;

protected:
    static QList<ScalarsForColorMapping *> newInstances(const QList<DataObject *> & dataObjects);

    void updateBounds() override;

    QByteArray arraySectionName(DataObject * dataObject);

private:
    static const bool s_registered;

    QMap<DataObject *, vtkIdType> m_dataObjectToArrayIndex;

    vtkFloatArray * m_dataArray;
};
