#pragma once

#include <QList>
#include <QMap>
#include <QObject>
#include <QString>

#include <vtkSmartPointer.h>

#include <core/core_api.h>


class vtkActor;
class vtkMapper;
class vtkArrowSource;
class vtkGlyph3D;

namespace reflectionzeug
{
    class PropertyGroup;
}

class DataObject;
class RenderedData;


/**
Abstract base class for vectors that can be mapped to polygonal surfaces.
*/
class CORE_API VectorMappingData : public QObject
{
    Q_OBJECT

public:
    friend class VectorMappingRegistry;

public:
    virtual ~VectorMappingData() = 0;

    virtual QString name() const = 0;

    bool isVisible() const;
    void setVisible(bool enabled);

    /** when mapping arrays that have more tuples than cells (triangles) exist in the data set:
        return highest possible index that can be used as first index in the vector array. */
    virtual vtkIdType maximumStartingIndex();
    vtkIdType startingIndex() const;
    void setStartingIndex(vtkIdType index);

    float arrowLength() const;
    void setArrowLength(float length);

    float arrowRadius() const;
    void setArrowRadius(float radius);

    float arrowTipLength() const;
    void setArrowTipLength(float tipLength);

    vtkActor * actor();

    virtual reflectionzeug::PropertyGroup * createPropertyGroup();

signals:
    void geometryChanged();

protected:
    template<typename SubClass>
    /** default function for Registry::MappingCreator, returning a single mapping instance */
    static QList<VectorMappingData *> newInstance(RenderedData * renderedData);
    explicit VectorMappingData(RenderedData * renderedData);

    virtual void initialize();

    virtual bool isValid() const;

    DataObject * dataObject();
    RenderedData * renderedData();
    vtkGlyph3D * arrowGlyph();

    virtual void visibilityChangedEvent();
    virtual void startingIndexChangedEvent();

private:
    RenderedData * m_renderedData;

    bool m_isVisible;

    vtkIdType m_startingIndex;

    vtkSmartPointer<vtkArrowSource> m_arrowSource;
    vtkSmartPointer<vtkGlyph3D> m_arrowGlyph;
    vtkSmartPointer<vtkMapper> m_mapper;
    vtkSmartPointer<vtkActor> m_actor;

protected:
    bool m_isValid;

};

#include "VectorMappingData.hpp"
