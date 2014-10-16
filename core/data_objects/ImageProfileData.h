#pragma once

#include <vtkSmartPointer.h>

#include <core/data_objects/DataObject.h>


class vtkLineSource;
class vtkTransformFilter;
class vtkWarpScalar;
class ImageDataObject;


class CORE_API ImageProfileData : public DataObject
{
public:
    ImageProfileData(const QString & name, ImageDataObject * imageData);

    bool is3D() const override;
    RenderedData * createRendered() override;

    QString dataTypeName() const override;

    vtkDataSet * processedDataSet() override;
    vtkAlgorithmOutput * processedOutputPort() override;

    const QString & scalarsName() const;

    const double * scalarRange();
    int numberOfScalars();

protected:
    QVtkTableModel * createTableModel() override;

    friend class RenderViewStrategyImage2D;
    const double * point1() const;
    const double * point2() const;
    void setPoints(double point1[3], double point2[3]);

private:
    ImageDataObject * m_imageData;
    QString m_scalarsName;

    vtkSmartPointer<vtkLineSource> m_probeLine;
    vtkSmartPointer<vtkTransformFilter> m_transform;
    vtkSmartPointer<vtkWarpScalar> m_graphLine;
};
