/*
 * GeohazardVis
 * Copyright (C) 2017 Karsten Tausche <geodev@posteo.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <map>
#include <memory>
#include <vector>

#include <QObject>

#include <vtkSmartPointer.h>

#include <core/core_api.h>


class QString;
class vtkActor;
class vtkAlgorithm;
class vtkAlgorithmOutput;
class vtkGlyph3D;
class vtkMapper;
class vtkProp;
class vtkProp3D;
class vtkScalarsToColors;

namespace reflectionzeug
{
    class PropertyGroup;
}

class ColorMappingData;
class DataObject;
enum class IndexType;
class RenderedData;


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

    /** Location of visualized attribute (points, cells, ...) */
    virtual IndexType scalarsAssociation() const = 0;

    bool isVisible() const;
    void setVisible(bool visible);

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
    std::map<Representation, vtkSmartPointer<vtkAlgorithm>> m_arrowSources;

    vtkSmartPointer<vtkGlyph3D> m_arrowGlyph;
    vtkSmartPointer<vtkMapper> m_mapper;
    vtkSmartPointer<vtkActor> m_actor;

    ColorMappingData * m_colorMappingData;
    vtkSmartPointer<vtkScalarsToColors> m_colorMappingGradient;

protected:
    bool m_isValid;

private:
    Q_DISABLE_COPY(GlyphMappingData)
};

#include "GlyphMappingData.hpp"
