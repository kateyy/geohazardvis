#pragma once

#include <vtkWeakPointer.h>

#include "ScalarsForColorMapping.h"


class vtkElevationFilter;

class PolyDataObject;


class AbstractCoordinateValueMapping : public ScalarsForColorMapping
{
public:
    AbstractCoordinateValueMapping(const QList<DataObject *> & dataObjects);
    ~AbstractCoordinateValueMapping() override;

    bool usesGradients() const override;

    /** create a filter to map values to color, applying current min/max settings
      * Stores a reference to the filter, to update min/max values on demand. */
    vtkAlgorithm * createFilter();

protected:
    bool isValid() const override;
    void minMaxChanged() override;

protected:
    QList<PolyDataObject *> m_dataObjects;
    QList<vtkWeakPointer<vtkElevationFilter>> m_filters;
};


class CoordinateXValueMapping : public AbstractCoordinateValueMapping
{
public:
    CoordinateXValueMapping(const QList<DataObject *> & dataObjects);

    QString name() const override;

protected:
    void updateBounds() override;
    void minMaxChanged() override;

private:
    static const QString s_name;
    static const bool s_registered;
};


class CoordinateYValueMapping : public AbstractCoordinateValueMapping
{
public:
    CoordinateYValueMapping(const QList<DataObject *> & dataObjects);

     QString name() const override;

protected:
    void updateBounds() override;
    void minMaxChanged() override;

private:
    static const QString s_name;
    static const bool s_registered;
};


class CoordinateZValueMapping : public AbstractCoordinateValueMapping
{
public:
    CoordinateZValueMapping(const QList<DataObject *> & dataObjects);

    QString name() const override;

protected:
    void updateBounds() override;
    void minMaxChanged() override;

private:
    static const QString s_name;
    static const bool s_registered;
};
