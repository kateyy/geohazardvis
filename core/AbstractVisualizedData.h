#pragma once

#include <memory>

#include <QObject>

#include <core/core_api.h>


class vtkAlgorithmOutput;
class vtkDataSet;
class vtkInformation;
class vtkScalarsToColors;
namespace reflectionzeug
{
    class PropertyGroup;
}

class AbstractVisualizedData_private;
class ColorMapping;
enum class ContentType;
class DataObject;
class ColorMappingData;


/**
Base class for visual representations of data objects.
A data object may be visualized in multiple views, each holding its own AbstractVisualizedData
instance, referring to the data object.
This base class does not define how the visualization is implemented, it only defines the following
common features:
    - Color Mapping:
        Each visualization own a ColorMapping that provides ColorMappingData subclasses appropriate
        for this visualization. At least a default (no-op) mapping is always available.
    - Output Ports:
        Visualization may provide multiple output ports for different parts/kinds of visual objects.
    - Information Reference:
        Visualization reference themselves in their visualization pipeline using the
        setupInformation function. This allows to quickly link from a visible object, e.g., after
        picking, back to the related AbstractVisualizedData instance.
*/
class CORE_API AbstractVisualizedData : public QObject
{
    Q_OBJECT

public:
    AbstractVisualizedData(ContentType contentType, DataObject & dataObject);
    ~AbstractVisualizedData() override;

    ContentType contentType() const;

    DataObject & dataObject();
    const DataObject & dataObject() const;

    bool isVisible() const;
    void setVisible(bool visible);

    virtual std::unique_ptr<reflectionzeug::PropertyGroup> createConfigGroup() = 0;

    /** Color mapping used for this visualization. If it doesn't current has a color mapping, a new
      * one will be created. */
    ColorMapping & colorMapping();

    /** This is used by the ColorMapping to set the current scalars. Don't call this from anywhere else. */
    void setScalarsForColorMapping(ColorMappingData * scalars);
    /** Set gradient that will be applied to colored geometries.
    * ColorMappingData are responsible for gradient configuration. */
    void setColorMappingGradient(vtkScalarsToColors * gradient);

    virtual int numberOfColorMappingInputs() const;
    virtual vtkAlgorithmOutput * colorMappingInput(int connection = 0);
    vtkDataSet * colorMappingInputData(int connection = 0);

    virtual int defaultVisualizationPort() const;

    /** store data object name and pointers in the information object */
    static void setupInformation(vtkInformation & information, AbstractVisualizedData & visualization);

    static AbstractVisualizedData * readPointer(vtkInformation & information);
    static void storePointer(vtkInformation & information, AbstractVisualizedData * visualization);

signals:
    void visibilityChanged(bool visible);
    /** Emitted when the underlaying data or its visible representation was modified */
    void geometryChanged();

protected:
    /** Subclasses can override this method to set initial parameters on a newly created color mapping */
    virtual void setupColorMapping(ColorMapping & colorMapping);
    virtual void visibilityChangedEvent(bool visible);
    virtual void scalarsForColorMappingChangedEvent();
    virtual void colorMappingGradientChangedEvent();

    /** Convenience method to return the current data mapped to colors. This may be nullptr */
    ColorMappingData * currentColorMappingData();
    vtkScalarsToColors * currentColorMappingGradient();


private:
    std::unique_ptr<AbstractVisualizedData_private> d_ptr;

private:
    Q_DISABLE_COPY(AbstractVisualizedData)
};
