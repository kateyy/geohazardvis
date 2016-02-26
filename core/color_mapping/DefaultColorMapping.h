#pragma once

#include <core/color_mapping/ColorMappingData.h>


/** "Null"-Color Mapping that is used when color mapping is disabled.

This ColorMappingData is not directly selectable by the user, thus is it not registered in the ColorMappingRegistry.
Instead, it is activated when disabling color mapping and will allows the user to configure surface colors directly.
*/
class CORE_API DefaultColorMapping : public ColorMappingData
{
public:
    explicit DefaultColorMapping(const QList<AbstractVisualizedData *> & visualizedData);

    QString name() const override;

    void configureMapper(AbstractVisualizedData * visualizedData, vtkAbstractMapper * mapper) override;

protected:
    QMap<int, QPair<double, double>> updateBounds() override;

private:
    static const QString s_name;
};
