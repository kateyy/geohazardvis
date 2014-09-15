#pragma once

#include <vtkType.h>

#include <QMap>

#include <core/scalar_mapping/ScalarsForColorMapping.h>


class vtkFloatArray;

class PolyDataObject;


class CORE_API RawArrayComponentMapping : public ScalarsForColorMapping
{
public:
    RawArrayComponentMapping(const QList<DataObject *> & dataObjects, vtkFloatArray * dataArray, vtkIdType component);
    ~RawArrayComponentMapping() override;

    QString name() const override;

    vtkAlgorithm * createFilter(DataObject * dataObject) override;
    bool usesFilter() const override;

    void configureDataObjectAndMapper(DataObject * dataObject, vtkMapper * mapper) override;

protected:
    static QList<ScalarsForColorMapping *> newInstances(const QList<DataObject *> & dataObjects);

    void updateBounds() override;
    bool isValid() const override;

    QByteArray arraySectionName(DataObject * dataObject);

private:
    static const bool s_registered;

    bool m_valid;

    QMap<DataObject *, vtkIdType> m_dataObjectToArrayIndex;

    vtkFloatArray * m_dataArray;
    vtkIdType m_component;
};
