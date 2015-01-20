#pragma once

#include <core/color_mapping/ColorMappingData.h>


class CORE_API VertexComponentColorMapping : public ColorMappingData
{
public:
    VertexComponentColorMapping(const QList<AbstractVisualizedData*> & visualizedData, vtkIdType component);
    ~VertexComponentColorMapping() override;

    QString name() const override;

    vtkAlgorithm * createFilter(AbstractVisualizedData * visualizedData) override;
    bool usesFilter() const override;

    void configureMapper(AbstractVisualizedData * visualizedData, vtkMapper * mapper) override;

protected:
    static QList<ColorMappingData *> newInstances(const QList<AbstractVisualizedData*> & visualizedData);

    QMap<vtkIdType, QPair<double, double>> updateBounds() override;

private:
    static const bool s_isRegistered;

    const vtkIdType m_component;
};
