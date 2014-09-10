#pragma once

#include <core/scalar_mapping/ScalarsForColorMapping.h>


class ImageDataObject;


class CORE_API GridColorMapping : public ScalarsForColorMapping
{
public:
    GridColorMapping(const QList<DataObject *> & dataObjects);
    ~GridColorMapping() override;

    QString name() const override;

protected:
    virtual void updateBounds() override;

    virtual bool isValid() const override;

private:
    const bool m_isValid;

    ImageDataObject * m_dataObject;

    static const QString s_name;
    static const bool s_registered;
};
