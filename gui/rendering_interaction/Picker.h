#pragma once

#include <QStringList>

#include <vtkSmartPointer.h>

#include <core/types.h>

#include <gui/gui_api.h>


class QTextStream;
class vtkAbstractMapper3D;
class vtkCellPicker;
class vtkPointPicker;
class vtkRenderer;
class vtkVector2i;

class AbstractVisualizedData;
class DataObject;
class PolyDataObject;


class GUI_API Picker
{
public:
    Picker();
    virtual ~Picker();

    void pick(const vtkVector2i & clickPosXY, vtkRenderer & renderer);

    QStringList pickedObjectInfo();

    vtkIdType pickedIndex() const;
    IndexType pickedIndexType() const;
    DataObject * pickedDataObject() const;
    AbstractVisualizedData * pickedVisualizedData() const;


private:
    void appendPolyDataInfo(QTextStream & stream, PolyDataObject & polyData);
    void appendImageDataInfo(QTextStream & stream);
    void appendGlyphInfo(QTextStream & stream);
    
private:
    vtkSmartPointer<vtkPointPicker> m_pointPicker;
    vtkSmartPointer<vtkCellPicker> m_cellPicker;

    QStringList m_pickedObjectInfo;
    vtkIdType m_pickedIndex;
    IndexType m_pickedIndexType;
    DataObject * m_pickedDataObject;
    AbstractVisualizedData * m_pickedVisualizedData;
};
