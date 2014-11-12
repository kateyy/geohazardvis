#pragma once

#include <QList>
#include <QMap>
#include <QObject>
#include <QString>

#include <vtkSmartPointer.h>

#include <core/core_api.h>


class vtkActor;
class vtkProp;
class vtkAlgorithm;
class vtkMapper;
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

    enum class Representation
    {
        Line,
        SimpleArrow,
        CylindricArrow
    };

public:
    virtual ~VectorMappingData() = 0;

    virtual QString name() const = 0;

    bool isVisible() const;
    void setVisible(bool enabled);

    Representation representation() const;
    void setRepresentation(Representation representation);

    const double * color() const;
    void color(double color[3]) const;
    void setColor(double r, double g, double b);

    float arrowLength() const;
    void setArrowLength(float length);

    float arrowRadius() const;
    void setArrowRadius(float radius);

    float arrowTipLength() const;
    void setArrowTipLength(float tipLength);

    unsigned lineWidth() const;
    void setLineWidth(unsigned lineWidth);

    vtkActor * actor();
    vtkProp * viewProp();

    virtual reflectionzeug::PropertyGroup * createPropertyGroup();

signals:
    void geometryChanged();

protected:
    template<typename SubClass>
    /** default function for Registry::MappingCreator, returning a single mapping instance */
    static QList<VectorMappingData *> newInstance(RenderedData * renderedData);
    explicit VectorMappingData(RenderedData * renderedData);

    virtual void initialize();

    bool isValid() const;

    DataObject * dataObject();
    RenderedData * renderedData();
    vtkGlyph3D * arrowGlyph();

    virtual void visibilityChangedEvent();
    virtual void startingIndexChangedEvent();

private:
    RenderedData * m_renderedData;

    bool m_isVisible;

    Representation m_representation;
    QMap<Representation, vtkSmartPointer<vtkAlgorithm>> m_arrowSources;

    vtkSmartPointer<vtkGlyph3D> m_arrowGlyph;
    vtkSmartPointer<vtkMapper> m_mapper;
    vtkSmartPointer<vtkActor> m_actor;

protected:
    bool m_isValid;

};

#include "VectorMappingData.hpp"
