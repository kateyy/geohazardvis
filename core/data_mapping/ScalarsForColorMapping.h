#pragma once

#include <QList>
#include <QMap>
#include <QString>

#include <core/core_api.h>


class DataObject;


/**
Abstract base class for scalars that can be used for surface color mappings.
Scalar mappings extract relevant data from an input data objects and supply their defined scalar values.
A subrange of values may be used for the color mapping (setMinValue, setMaxValue).
*/
class CORE_API ScalarsForColorMapping
{
public:
    friend class ScalarsForColorMappingRegistry;

    template<typename SubClass>
    static ScalarsForColorMapping * newInstance(const QList<DataObject*> & dataObjects);

public:
    explicit ScalarsForColorMapping(const QList<DataObject *> & dataObjects);
    virtual ~ScalarsForColorMapping() = 0;


    /** minimal value in the data set */
    double dataMinValue() const;
    /** maximal value in the data set */
    double dataMaxValue() const;

    /** minimal value used for scalar mapping (clamped to [dataMinValue, dataMaxValue]) */
    double minValue() const;
    void setMinValue(double value);
    /** maximal value used for scalar mapping (clamped to [dataMinValue, dataMaxValue]) */
    double maxValue() const;
    void setMaxValue(double value);

    virtual QString name() const = 0;

    virtual bool usesGradients() const = 0;

protected:
    virtual void initialize();
    virtual void updateBounds() = 0;

    /** check whether these scalar extraction is applicable for the data objects it was created with */
    virtual bool isValid() const = 0;

protected:
    double m_dataMinValue;
    double m_dataMaxValue;
    double m_minValue;
    double m_maxValue;
};

#include "ScalarsForColorMapping.hpp"
