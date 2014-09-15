#pragma once

#include <vtkType.h>

#include <core/scalar_mapping/ScalarsForColorMapping.h>


class CORE_API AbstractArrayComponentMapping : public ScalarsForColorMapping
{
public:
    AbstractArrayComponentMapping(const QList<DataObject *> & dataObjects, QString dataArrayName, vtkIdType component);

    QString name() const override;

protected:
    bool isValid() const override;

    static vtkInformationIntegerKey * ArrayIsAuxiliaryKey();

protected:
    bool m_isValid;

    QString m_dataArrayName;
    vtkIdType m_component;
    vtkIdType m_arrayNumComponents;
};
