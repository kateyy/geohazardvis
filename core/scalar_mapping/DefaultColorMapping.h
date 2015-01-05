#pragma once

#include <core/scalar_mapping/ScalarsForColorMapping.h>


class CORE_API DefaultColorMapping : public ScalarsForColorMapping
{
public:
    DefaultColorMapping(const QList<AbstractVisualizedData *> & visualizedData);
    ~DefaultColorMapping() override;

    QString name() const override;

    void configureMapper(AbstractVisualizedData * visualizedData, vtkMapper * mapper) override;

protected:
    virtual void updateBounds() override;

private:
    static const QString s_name;
    static const bool s_isRegistered;
};
