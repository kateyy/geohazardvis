#pragma once

#include <vtkSmartPointer.h>

#include <core/types.h>
#include <gui/gui_api.h>


class vtkDataSet;
class vtkImageData;
class vtkPolyData;
class vtkRenderer;

class DataObject;
class AbstractVisualizedData;


class GUI_API CameraDolly
{
public:
    void setRenderer(vtkRenderer * renderer);
    vtkRenderer * renderer() const;

    /** Move the camera over time to look at the specified primitive in the visualization. */
    void moveTo(AbstractVisualizedData & visualization, vtkIdType index, IndexType indexType, bool overTime = true);
    void moveTo(DataObject & dataObject, vtkIdType index, IndexType indexType, bool overTime = true);
    void moveTo(vtkDataSet & dataSet, vtkIdType index, IndexType indexType, bool overTime = true);


    void moveToPoly(vtkPolyData & poly, vtkIdType index, IndexType indexType, bool overTime);
    void moveToImage(vtkImageData & image, vtkIdType index, IndexType indexType, bool overTime);

private:
    vtkSmartPointer<vtkRenderer> m_renderer;
};
