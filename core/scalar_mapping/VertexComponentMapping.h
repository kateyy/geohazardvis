#pragma once

#include <core/scalar_mapping/ScalarsForColorMapping.h>


class CORE_API VertexComponentMapping : public ScalarsForColorMapping
{
public:
    VertexComponentMapping(const QList<DataObject *> & dataObjects, vtkIdType component);
    ~VertexComponentMapping() override;

    QString name() const override;

    vtkAlgorithm * createFilter(DataObject * dataObject);
    bool usesFilter() const;

protected:
    static QList<ScalarsForColorMapping *> newInstances(const QList<DataObject *> & dataObjects);

    void updateBounds() override;

    bool isValid() const override;

private:
    static const bool s_registered;

    const vtkIdType m_component;
};