#pragma once

#include <vtkWeakPointer.h>

#include <core/scalar_mapping/ScalarsForColorMapping.h>


class vtkElevationFilter;

class PolyDataObject;


class CORE_API AbstractCoordinateValueMapping : public ScalarsForColorMapping
{
public:
    AbstractCoordinateValueMapping(const QList<DataObject *> & dataObjects);
    ~AbstractCoordinateValueMapping() override;

    bool usesGradients() const override;

    /** create a filter to map values to color, applying current min/max settings
      * Stores a reference to the filter, to update min/max values on demand. */
    vtkAlgorithm * createFilter() override;

protected:
    bool isValid() const override;

    void minMaxChangedEvent() override;

protected:
    QList<PolyDataObject *> m_dataObjects;
    QList<vtkWeakPointer<vtkElevationFilter>> m_filters;
};


class CORE_API CoordinateXValueMapping : public AbstractCoordinateValueMapping
{
public:
    CoordinateXValueMapping(const QList<DataObject *> & dataObjects);

    QString name() const override;

protected:
    void updateBounds() override;
    void minMaxChangedEvent() override;

private:
    static const QString s_name;
    static const bool s_registered;
};


class CORE_API CoordinateYValueMapping : public AbstractCoordinateValueMapping
{
public:
    CoordinateYValueMapping(const QList<DataObject *> & dataObjects);

     QString name() const override;

protected:
    void updateBounds() override;
    void minMaxChangedEvent() override;

private:
    static const QString s_name;
    static const bool s_registered;
};


class CORE_API CoordinateZValueMapping : public AbstractCoordinateValueMapping
{
public:
    CoordinateZValueMapping(const QList<DataObject *> & dataObjects);

    QString name() const override;

protected:
    void updateBounds() override;
    void minMaxChangedEvent() override;

private:
    static const QString s_name;
    static const bool s_registered;
};
