#pragma once

#include <memory>

#include <QObject>

#include <vtkSmartPointer.h>

#include <core/core_api.h>


class vtkAlgorithmOutput;
class vtkDataSet;
class vtkInformation;
class vtkInformationIntegerPointerKey;
class vtkScalarsToColors;
namespace reflectionzeug
{
    class PropertyGroup;
}

class ColorMapping;
enum class ContentType;
class DataObject;
class ColorMappingData;


class CORE_API AbstractVisualizedData : public QObject
{
    Q_OBJECT

public:
    AbstractVisualizedData(ContentType contentType, DataObject & dataObject, QObject * parent = nullptr);
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

    /** store data object name and pointers in the information object */
    static void setupInformation(vtkInformation & information, AbstractVisualizedData & visualization);

    static AbstractVisualizedData * readPointer(vtkInformation & information);
    static void storePointer(vtkInformation & information, AbstractVisualizedData * visualization);

signals:
    void visibilityChanged(bool visible);
    void geometryChanged();

protected:
    virtual void visibilityChangedEvent(bool visible);
    virtual void scalarsForColorMappingChangedEvent();
    virtual void colorMappingGradientChangedEvent();

protected:
    ColorMappingData * m_colorMappingData;
    vtkSmartPointer<vtkScalarsToColors> m_gradient;

private:
    static vtkInformationIntegerPointerKey * VisualizedDataKey();

private:
    const ContentType m_contentType;
    DataObject & m_dataObject;

    bool m_isVisible;

    /** Color mappings can be shared between multiple visualizations. */
    std::unique_ptr<ColorMapping> m_colorMapping;
};
