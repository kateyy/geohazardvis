#pragma once

#include <vtkSmartPointer.h>

#include <core/data_objects/DataObject.h>

class vtkLineSource;
class ImageDataObject;


class CORE_API ImageProfileData : public DataObject
{
public:
    ImageProfileData(const QString & name, ImageDataObject * imageData, double point1[3], double point2[3]);

    bool is3D() const override;
    RenderedData * createRendered() override;

    QString dataTypeName() const override;

    vtkDataSet * processedDataSet() override;
    vtkAlgorithmOutput * processedOutputPort() override;

    const QString & scalarsName() const;
    const double * point1() const;
    const double * point2() const;

protected:
    QVtkTableModel * createTableModel() override;

private:
    ImageDataObject * m_imageData;
    QString m_scalarsName;
    double m_point1[3];
    double m_point2[3];

    vtkSmartPointer<vtkLineSource> m_graphLine;
};
