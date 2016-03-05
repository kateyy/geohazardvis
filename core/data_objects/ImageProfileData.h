#pragma once

#include <vtkSmartPointer.h>
#include <vtkVector.h>

#include <core/types.h>
#include <core/data_objects/DataObject.h>


class vtkLineSource;
class vtkProbeFilter;
class vtkTransformPolyDataFilter;
class vtkWarpScalar;

enum class IndexType;
class LinearSelectorXY;


/**
* Probes the source data along a line defined by two points on the XY-plane and creates a plot for the interpolated data.
*/
class CORE_API ImageProfileData : public DataObject
{
public:
    /** Create profile with given specifications
      * @param name Object name, see DataObject API
      * @param sourceData source data object that will be probed
      * @param scalarsName Scalars to probe in the source data object 
      * @param scalarsLocation Specifies whether to probe point or cell scalars 
      * @param vectorComponent For multi component scalars/vectors, specify which component will be extracted */
    ImageProfileData(
        const QString & name, 
        DataObject & sourceData, 
        const QString & scalarsName,
        IndexType scalarsLocation,
        vtkIdType vectorComponent);

    /** @return whether valid scalar data was found in the constructor. Otherwise, just delete your instance.. */
    bool isValid() const;

    bool is3D() const override;
    std::unique_ptr<Context2DData> createContextData() override;

    const QString & dataTypeName() const override;
    static const QString & dataTypeName_s();

    vtkDataSet * processedDataSet() override;
    vtkAlgorithmOutput * processedOutputPort() override;

    const QString & abscissa() const;

    const DataObject & sourceData() const;
    const QString & scalarsName() const;
    IndexType scalarsLocation() const;
    vtkIdType vectorComponent() const;

    const double * scalarRange();
    int numberOfScalars();

    /** @return X,Y-coordinates for the first point */
    const vtkVector2d & point1() const;
    /** @return X,Y-coordinates for the second point */
    const vtkVector2d & point2() const;
    void setPoints(const vtkVector2d & point1, const vtkVector2d & point2);

protected:
    std::unique_ptr<QVtkTableModel> createTableModel() override;

private:
    bool m_isValid;
    DataObject & m_sourceData;
    QString m_abscissa;
    QString m_scalarsName;
    IndexType m_scalarsLocation;
    vtkIdType m_vectorComponent;

    vtkVector2d m_point1;
    vtkVector2d m_point2;

    // extraction from vtkImageData
    const bool m_inputIsImage;
    vtkSmartPointer<vtkLineSource> m_probeLine;
    vtkSmartPointer<vtkProbeFilter> m_imageProbe;

    // extraction from vtkPolyData
    vtkSmartPointer<LinearSelectorXY> m_polyDataPointsSelector;

    vtkSmartPointer<vtkTransformPolyDataFilter> m_outputTransformation;
    vtkSmartPointer<vtkWarpScalar> m_graphLine;
};
