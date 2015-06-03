#pragma once

#include <core/color_mapping/ColorMappingData.h>


class CORE_API DefaultColorMapping : public ColorMappingData
{
public:
    DefaultColorMapping(const QList<AbstractVisualizedData *> & visualizedData);

    QString name() const override;

    void configureMapper(AbstractVisualizedData * visualizedData, vtkAbstractMapper * mapper) override;

protected:
    QMap<int, QPair<double, double>> updateBounds() override;

private:
    static const QString s_name;
    static const bool s_isRegistered;
};
