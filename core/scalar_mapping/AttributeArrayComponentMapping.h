#pragma once

#include <vtkType.h>

#include <core/scalar_mapping/ScalarsForColorMapping.h>


class vtkPolyData;

class AttributeVectorData;
class PolyDataObject;


class CORE_API AttributeArrayComponentMapping : public ScalarsForColorMapping
{
public:
    AttributeArrayComponentMapping(const QList<DataObject *> & dataObjects, AttributeVectorData * attributeVector, vtkIdType component);
    ~AttributeArrayComponentMapping() override;

    QString name() const override;
    bool usesGradients() const override;

protected:
    static QList<ScalarsForColorMapping *> newInstances(const QList<DataObject *> & dataObjects);
    void initialize() override;

    void updateBounds() override;
    bool isValid() const override;
    void minMaxChangedEvent() override;

private:
    static const bool s_registered;

    bool m_valid;

    AttributeVectorData * m_attributeVector;
    vtkIdType m_component;

    vtkPolyData * m_polyData;
};
