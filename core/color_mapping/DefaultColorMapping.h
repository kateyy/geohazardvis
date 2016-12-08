#pragma once

#include <core/color_mapping/ColorMappingData.h>


/** "Null"-Color Mapping that is used when color mapping is disabled.

This ColorMappingData is not directly selectable by the user, thus is it not registered in the ColorMappingRegistry.
Instead, it is activated when disabling color mapping and will allows the user to configure surface colors directly.
*/
class CORE_API DefaultColorMapping : public ColorMappingData
{
public:
    explicit DefaultColorMapping(const std::vector<AbstractVisualizedData *> & visualizedData);
    ~DefaultColorMapping() override;

    QString name() const override;

    void configureMapper(AbstractVisualizedData & visualizedData, vtkAbstractMapper & mapper, unsigned int port = 0) override;

protected:
    std::vector<ValueRange<>> updateBounds() override;

private:
    static const QString s_name;

private:
    Q_DISABLE_COPY(DefaultColorMapping)
};
