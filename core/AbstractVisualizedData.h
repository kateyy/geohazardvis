#pragma once

#include <QObject>

#include <vtkSmartPointer.h>

#include <core/core_api.h>


class vtkScalarsToColors;
namespace reflectionzeug
{
    class PropertyGroup;
}

enum class ContentType;
class DataObject;
class ScalarsForColorMapping;


class CORE_API AbstractVisualizedData : public QObject
{
    Q_OBJECT

public:
    AbstractVisualizedData(ContentType contentType, DataObject * dataObject, QObject * parent = nullptr);

    ContentType contentType() const;

    DataObject * dataObject();
    const DataObject * dataObject() const;

    bool isVisible() const;
    void setVisible(bool visible);

    virtual reflectionzeug::PropertyGroup * createConfigGroup() = 0;

    /** set scalars that will configure color mapping for this data */
    void setScalarsForColorMapping(ScalarsForColorMapping * scalars);
    /** Set gradient that will be applied to colored geometries.
    * ScalarsForColorMapping are responsible for gradient configuration. */
    void setColorMappingGradient(vtkScalarsToColors * gradient);

signals:
    void visibilityChanged(bool visible);
    void geometryChanged();

protected:
    virtual void visibilityChangedEvent(bool visible);
    virtual void scalarsForColorMappingChangedEvent();
    virtual void colorMappingGradientChangedEvent();

protected:
    ScalarsForColorMapping * m_scalars;
    vtkSmartPointer<vtkScalarsToColors> m_gradient;

private:
    const ContentType m_contentType;
    DataObject * m_dataObject;

    bool m_isVisible;
};
