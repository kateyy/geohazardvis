#pragma once

#include <core/scalar_mapping/ScalarsForColorMapping.h>


class CORE_API VertexComponentMapping : public ScalarsForColorMapping
{
public:
    VertexComponentMapping(const QList<AbstractVisualizedData*> & visualizedData, vtkIdType component);
    ~VertexComponentMapping() override;

    QString name() const override;

    vtkAlgorithm * createFilter(AbstractVisualizedData * visualizedData) override;
    bool usesFilter() const override;

    void configureMapper(AbstractVisualizedData * visualizedData, vtkMapper * mapper) override;

protected:
    static QList<ScalarsForColorMapping *> newInstances(const QList<AbstractVisualizedData*> & visualizedData);

    void updateBounds() override;

private:
    static const bool s_registered;

    const vtkIdType m_component;
};
