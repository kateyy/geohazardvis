#pragma once

#include "ScalarsForColorMapping.h"


class DefaultColorMapping : public ScalarsForColorMapping
{
public:
    DefaultColorMapping(const QList<DataObject *> & dataObjects);
    ~DefaultColorMapping() override;

    virtual QString name() const override;
    virtual bool usesGradients() const override;

protected:
    virtual void updateBounds() override;

    virtual bool isValid() const override;

private:
    static const QString s_name;
    static const bool s_registered;
};
