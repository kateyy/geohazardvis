#pragma once

#include "ScalarsForColorMapping.h"


class ImageDataObject;


class GridColorMapping : public ScalarsForColorMapping
{
public:
    GridColorMapping(const QList<DataObject *> & dataObjects);
    ~GridColorMapping() override;

    virtual QString name() const override;
    virtual bool usesGradients() const override;

protected:
    virtual void updateBounds() override;

    virtual bool isValid() const override;

private:
    const bool m_isValid;

    ImageDataObject * m_dataObject;

    static const QString s_name;
    static const bool s_registered;
};
