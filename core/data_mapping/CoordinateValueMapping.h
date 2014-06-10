#pragma once

#include "ScalarsForColorMapping.h"


class PolyDataObject;


class AbstractCoordinateValueMapping : public ScalarsForColorMapping
{
public:
    AbstractCoordinateValueMapping(const QList<DataObject *> & dataObjects);
    ~AbstractCoordinateValueMapping() override;

    virtual bool usesGradients() const override;

protected:
    virtual bool isValid() const override;

    QList<PolyDataObject *> m_dataObjects;
};


class CoordinateXValueMapping : public AbstractCoordinateValueMapping
{
public:
    CoordinateXValueMapping(const QList<DataObject *> & dataObjects);

    virtual QString name() const override;

protected:
    virtual void updateBounds() override;
};


class CoordinateYValueMapping : public AbstractCoordinateValueMapping
{
public:
    CoordinateYValueMapping(const QList<DataObject *> & dataObjects);

    virtual QString name() const override;

protected:
    virtual void updateBounds() override;
};


class CoordinateZValueMapping : public AbstractCoordinateValueMapping
{
public:
    CoordinateZValueMapping(const QList<DataObject *> & dataObjects);

    virtual QString name() const override;

protected:
    virtual void updateBounds() override;
};
