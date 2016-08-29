#pragma once

#include <QString>

#include <vtkSmartPointer.h>

#include <core/types.h>
#include <gui/gui_api.h>


class QTextStream;
class vtkCellPicker;
class vtkDataArray;
class vtkImageSlice;
class vtkPointPicker;
class vtkPropPicker;
class vtkRenderer;
class vtkScalarsToColors;
class vtkVector2i;

class PolyDataObject;


class GUI_API Picker
{
public:
    Picker();
    virtual ~Picker();

    void pick(const vtkVector2i & clickPosXY, vtkRenderer & renderer);

    const QString & pickedObjectInfoString() const;

    const VisualizationSelection & pickedObjectInfo() const;

private:
    void appendPolyDataInfo(QTextStream & stream, PolyDataObject & polyData);
    void appendImageDataInfo(QTextStream & stream, vtkImageSlice & slice);
    void appendGenericPointInfo(QTextStream & stream);

    static void printScalarInfo(QTextStream & stream, vtkScalarsToColors * lut,
        vtkDataArray & scalars, vtkIdType pickedIndex);
    
private:
    vtkSmartPointer<vtkPropPicker> m_propPicker;
    vtkSmartPointer<vtkCellPicker> m_cellPicker;
    vtkSmartPointer<vtkPointPicker> m_pointPicker;

    QString m_pickedObjectInfoString;
    VisualizationSelection m_pickedObjectInfo;
};
