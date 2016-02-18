#pragma once

#include <memory>
#include <vector>

#include <QMap>
#include <QObject>

#include <vtkSmartPointer.h>

#include <core/core_api.h>


class QString;
class vtkActor;
class vtkAlgorithmOutput;
class vtkProp;
class vtkProp3D;
class vtkAlgorithm;
class vtkMapper;
class vtkGlyph3D;
class vtkScalarsToColors;

namespace reflectionzeug
{
    class PropertyGroup;
}

class DataObject;
class RenderedData;
class ColorMappingData;


/**
Abstract base class for vectors that can be mapped to polygonal surfaces.
*/
class CORE_API GlyphMappingData : public QObject
{
    Q_OBJECT

public:
    friend class GlyphMappingRegistry;

    enum class Representation
    {
        Line,
        SimpleArrow,
        CylindricArrow
    };

public:
    explicit GlyphMappingData(RenderedData & renderedData);

    DataObject & dataObject();
    RenderedData & renderedData();

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
    vtkProp3D * viewProp3D();

    virtual std::unique_ptr<reflectionzeug::PropertyGroup> createPropertyGroup();

    virtual vtkAlgorithmOutput * vectorDataOutputPort() = 0;

    ColorMappingData * colorMappingData();
    void setColorMappingData(ColorMappingData * colorMappingData);
    vtkScalarsToColors * colorMappingGradient();
    void setColorMappingGradient(vtkScalarsToColors * gradient);

signals:
    void geometryChanged();
    void visibilityChanged(bool isVisible);

protected:
    template<typename SubClass>
    /** default function for Registry::MappingCreator, returning a single mapping instance */
    static std::vector<std::unique_ptr<GlyphMappingData>> newInstance(RenderedData & renderedData);

    virtual void initialize();

    bool isValid() const;

    vtkGlyph3D * arrowGlyph();

    virtual void visibilityChangedEvent();
    virtual void colorMappingChangedEvent(ColorMappingData * colorMappingData);
    virtual void colorMappingGradientChangedEvent(vtkScalarsToColors * gradient);

private:
    RenderedData & m_renderedData;

    bool m_isVisible;

    Representation m_representation;
    QMap<Representation, vtkSmartPointer<vtkAlgorithm>> m_arrowSources;

    vtkSmartPointer<vtkGlyph3D> m_arrowGlyph;
    vtkSmartPointer<vtkMapper> m_mapper;
    vtkSmartPointer<vtkActor> m_actor;

    ColorMappingData * m_colorMappingData;
    vtkSmartPointer<vtkScalarsToColors> m_colorMappingGradient;

protected:
    bool m_isValid;

};

#include "GlyphMappingData.hpp"
