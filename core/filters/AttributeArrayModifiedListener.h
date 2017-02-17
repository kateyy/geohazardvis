#pragma once

#include <vtkPassThrough.h>

#include <QObject>

#include <core/types.h>


class vtkDataArray;


class CORE_API AttributeArrayModifiedListener : public QObject, public vtkPassThrough
{
    Q_OBJECT

public:
    vtkTypeMacro(AttributeArrayModifiedListener, vtkPassThrough);
    static AttributeArrayModifiedListener * New();

    vtkGetMacro(AttributeLocation, IndexType);
    void SetAttributeLocation(IndexType location);

signals:
    void attributeModified(vtkDataArray * array);

protected:
    AttributeArrayModifiedListener();
    ~AttributeArrayModifiedListener() override;

    int RequestData(
        vtkInformation * request,
        vtkInformationVector ** inputVector,
        vtkInformationVector * outputVector) override;

private:
    IndexType AttributeLocation;

    vtkMTimeType LastAttributeMTime;

private:
    AttributeArrayModifiedListener(const AttributeArrayModifiedListener &) = delete;
    void operator=(const AttributeArrayModifiedListener &) = delete;
};
