#pragma once

#include <core/color_mapping/ColorMappingData.h>

#include <QMap>


class CORE_API DEMReliefShading : public ColorMappingData
{
public:
    explicit DEMReliefShading(const QList<AbstractVisualizedData *> & visualizedData);
    ~DEMReliefShading() override;

    QString name() const override;
    QString scalarsName() const override;

    vtkSmartPointer<vtkAlgorithm> createFilter(AbstractVisualizedData * visualizedData, int connection = 0) override;
    bool usesFilter() const override;

protected:
    static std::vector<std::unique_ptr<ColorMappingData>> newInstance(const QList<AbstractVisualizedData *> & visualizedData);

    std::vector<ValueRange<>> updateBounds() override;

    vtkSmartPointer<vtkScalarsToColors> createOwnLookupTable() override;

    void lookupTableChangedEvent() override;
    void minMaxChangedEvent() override;

private:
    static const bool s_isRegistered;

    QMap<AbstractVisualizedData *, QMap<int, vtkSmartPointer<vtkAlgorithm>>> m_filters;
};
