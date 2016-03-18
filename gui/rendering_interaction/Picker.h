#pragma once

#include <QString>

#include <vtkSmartPointer.h>

#include <gui/gui_api.h>


class QTextStream;
class vtkCellPicker;
class vtkImageSlice;
class vtkPointPicker;
class vtkPropPicker;
class vtkRenderer;
class vtkVector2i;

class AbstractVisualizedData;
class DataObject;
enum class IndexType;
class PolyDataObject;


class GUI_API Picker
{
public:
    Picker();
    virtual ~Picker();

    void pick(const vtkVector2i & clickPosXY, vtkRenderer & renderer);

    const QString & pickedObjectInfo() const;

    vtkIdType pickedIndex() const;
    IndexType pickedIndexType() const;
    DataObject * pickedDataObject() const;
    AbstractVisualizedData * pickedVisualizedData() const;


private:
    void appendPolyDataInfo(QTextStream & stream, PolyDataObject & polyData);
    void appendImageDataInfo(QTextStream & stream, vtkImageSlice & slice);
    void appendGenericPointInfo(QTextStream & stream);
    
private:
    vtkSmartPointer<vtkPropPicker> m_propPicker;
    vtkSmartPointer<vtkCellPicker> m_cellPicker;
    vtkSmartPointer<vtkPointPicker> m_pointPicker;

    QString m_pickedObjectInfo;
    vtkIdType m_pickedIndex;
    IndexType m_pickedIndexType;
    DataObject * m_pickedDataObject;
    AbstractVisualizedData * m_pickedVisualizedData;
};
