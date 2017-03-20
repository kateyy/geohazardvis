#pragma once

#include <core/core_api.h>


class QString;
class vtkDataArray;
class vtkDataSet;
class vtkImageData;
class vtkPolyData;
template<typename T> class vtkSmartPointer;
enum class IndexType;


class CORE_API InterpolationHelper
{
public:
    /**
     * Interpolate an attribute from the sourceDataSet to the structure of the baseDataSet.
     *
     * If the structure of the input and the output matches, a faster code pass may be used. In the
     * simplest case, the selected attribute is extracted from the baseDataSet an not modified.
     * In this case, the original array is not copied.
     *
     * @param sourceDataSet contains the attribute that is to be interpolated.
     * @param sourceAttributeName name of the vtkDataArray stored in the point/cell attributes.
     * @param sourceLocation Location of the attribute in the source data set (point or cell data).
     * @param targetLocation Target location for the interpolation. This defined if the attribute
     *  will be interpolated to the points or the cells of the baseDataSet.
     * @return the interpolated attribute. If the interpolation was successful, the number of
     *  tuples matches the number of cells/points (see targetLocation) of the baseDataSet.
     *  Otherwise, this returns nullptr.
     */
    static vtkSmartPointer<vtkDataArray> interpolate(
        vtkDataSet & baseDataSet, 
        vtkDataSet & sourceDataSet,
        const QString & sourceAttributeName, 
        IndexType sourceLocation,
        IndexType targetLocation);

    enum class DataSetStructure
    {
        matching, mismatching, unkown
    };

    /**
     * Check if the structure of the supplied data sets match.
     * The following requirements must be met: The data sets have to have the same data type
     * (same subclass of vtkDataSet). Further, all points locations must match and must be defined
     * in the same order. This requirement must also be met for data set that contain cells.
     */
    static DataSetStructure isStructureMatching(vtkDataSet & lsh, vtkDataSet & rhs);
    static DataSetStructure isStructureMatching(vtkImageData & lsh, vtkImageData & rhs);
    static DataSetStructure isStructureMatching(vtkPolyData & lsh, vtkPolyData & rhs);

    /** Fast image on image interpolation for images with matching structure. */
    static vtkSmartPointer<vtkDataArray> fastInterpolateImageOnImage(
        vtkImageData & baseImage,
        vtkImageData & sourceImage,
        const QString & sourceAttributeName);

    /** Fast poly on poly interpolation for data sets with matching structure. */
    static vtkSmartPointer<vtkDataArray> fastInterpolatePolyOnPoly(
        vtkPolyData & basePoly,
        vtkPolyData & sourcePoly,
        const QString & sourceAttributeName,
        IndexType location);
};
