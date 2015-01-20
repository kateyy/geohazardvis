#pragma once

#include <core/color_mapping/ColorMappingData.h>


class CORE_API DefaultColorMapping : public ColorMappingData
{
public:
    DefaultColorMapping(const QList<AbstractVisualizedData *> & visualizedData);
    ~DefaultColorMapping() override;

    QString name() const override;

    void configureMapper(AbstractVisualizedData * visualizedData, vtkMapper * mapper) override;

protected:
    QMap<vtkIdType, QPair<double, double>> updateBounds() override;

private:
    static const QString s_name;
    static const bool s_isRegistered;
};
