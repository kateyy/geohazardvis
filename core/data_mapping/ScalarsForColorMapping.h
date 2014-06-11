#pragma once

#include <QList>
#include <QMap>
#include <QString>

class DataObject;


/**
Abstract base class for scalars that can be used for surface color mappings.
Scalar mappings extract relevant data from an input data objects and supply their defined scalar values.
*/
class ScalarsForColorMapping
{
public:
    friend class ScalarsForColorMappingRegistry;

    template<typename SubClass>
    static ScalarsForColorMapping * newInstance(const QList<DataObject*> & dataObjects);

public:
    explicit ScalarsForColorMapping(const QList<DataObject *> & dataObjects);
    virtual ~ScalarsForColorMapping() = 0;

    double minValue() const;
    double maxValue() const;

    virtual QString name() const = 0;

    virtual bool usesGradients() const = 0;

protected:
    virtual void initialize();
    virtual void updateBounds() = 0;

    /** check whether these scalar extraction is applicable for the data objects it was created with */
    virtual bool isValid() const = 0;

protected:
    double m_minValue;
    double m_maxValue;
};

#include "ScalarsForColorMapping.hpp"
