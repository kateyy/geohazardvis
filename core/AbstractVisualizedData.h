#pragma once

#include <QObject>

#include <vtkSmartPointer.h>

#include <core/core_api.h>


namespace reflectionzeug
{
    class PropertyGroup;
}

class DataObject;
enum class ContentType;


class CORE_API AbstractVisualizedData : public QObject
{
    Q_OBJECT

public:
    AbstractVisualizedData(DataObject * dataObject);

    DataObject * dataObject();
    const DataObject * dataObject() const;

    bool isVisible() const;
    void setVisible(bool visible);

    virtual reflectionzeug::PropertyGroup * createConfigGroup() = 0;

signals:
    void visibilityChanged(bool visible);
    void geometryChanged();

protected:
    virtual void visibilityChangedEvent(bool visible);

private:
    DataObject * m_dataObject;

    bool m_isVisible;
};
