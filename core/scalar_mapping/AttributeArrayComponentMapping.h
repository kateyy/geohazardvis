#pragma once

#include <vtkType.h>

#include <core/scalar_mapping/ScalarsForColorMapping.h>


class vtkFloatArray;

class PolyDataObject;


class CORE_API AttributeArrayComponentMapping : public ScalarsForColorMapping
{
public:
    AttributeArrayComponentMapping(const QList<DataObject *> & dataObjects, vtkFloatArray * dataArray, vtkIdType component);
    ~AttributeArrayComponentMapping() override;

    QString name() const override;

    void configureDataObjectAndMapper(DataObject * dataObject, vtkMapper * mapper) override;

protected:
    static QList<ScalarsForColorMapping *> newInstances(const QList<DataObject *> & dataObjects);

    void updateBounds() override;
    bool isValid() const override;

private:
    static const bool s_registered;

    bool m_valid;

    vtkFloatArray * m_dataArray;
    vtkIdType m_component;
};
