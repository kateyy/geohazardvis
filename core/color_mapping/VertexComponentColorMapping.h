#pragma once

#include <QMap>

#include <core/color_mapping/ColorMappingData.h>


class CORE_API VertexComponentColorMapping : public ColorMappingData
{
public:
    VertexComponentColorMapping(const QList<AbstractVisualizedData*> & visualizedData, int component);
    ~VertexComponentColorMapping() override;

    QString name() const override;

    vtkSmartPointer<vtkAlgorithm> createFilter(AbstractVisualizedData & visualizedData, int connection = 0) override;
    bool usesFilter() const override;

    void configureMapper(AbstractVisualizedData & visualizedData, vtkAbstractMapper & mapper) override;

protected:
    static std::vector<std::unique_ptr<ColorMappingData>> newInstances(const QList<AbstractVisualizedData*> & visualizedData);

    std::vector<ValueRange<>> updateBounds() override;

private:
    static const bool s_isRegistered;

    const int m_component;
};
