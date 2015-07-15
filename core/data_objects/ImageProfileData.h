#pragma once

#include <vtkSmartPointer.h>

#include <core/data_objects/DataObject.h>


class vtkLineSource;
class vtkProbeFilter;
class vtkTransformFilter;
class vtkWarpScalar;
class ImageDataObject;


class CORE_API ImageProfileData : public DataObject
{
public:
    ImageProfileData(const QString & name, ImageDataObject & imageData);

    bool is3D() const override;
    std::unique_ptr<Context2DData> createContextData() override;

    const QString & dataTypeName() const override;
    static const QString & dataTypeName_s();

    vtkDataSet * processedDataSet() override;
    vtkAlgorithmOutput * processedOutputPort() override;
    vtkDataSet * probedLine();
    vtkAlgorithmOutput * probedLineOuputPort();

    const QString & abscissa() const;
    const QString & scalarsName() const;

    const double * scalarRange();
    int numberOfScalars();

    const double * point1() const;
    const double * point2() const;
    void setPoints(double point1[3], double point2[3]);

protected:
    std::unique_ptr<QVtkTableModel> createTableModel() override;

private:
    ImageDataObject & m_imageData;
    QString m_abscissa;
    QString m_scalarsName;

    vtkSmartPointer<vtkLineSource> m_probeLine;
    vtkSmartPointer<vtkProbeFilter> m_probe;
    vtkSmartPointer<vtkTransformFilter> m_transform;
    vtkSmartPointer<vtkWarpScalar> m_graphLine;
};
