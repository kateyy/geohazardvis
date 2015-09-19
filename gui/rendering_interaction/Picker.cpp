#include "Picker.h"

#include <cassert>

#include <QDebug>

#include <vtkCellData.h>
#include <vtkCellPicker.h>
#include <vtkInformation.h>
#include <vtkInformationStringKey.h>
#include <vtkMapper.h>
#include <vtkPointData.h>
#include <vtkPointPicker.h>
#include <vtkPolyData.h>
#include <vtkScalarsToColors.h>
#include <vtkVector.h>

#include <core/AbstractVisualizedData.h>
#include <core/data_objects/ImageDataObject.h>
#include <core/data_objects/PolyDataObject.h>


Picker::Picker()
    : m_pointPicker(vtkSmartPointer<vtkPointPicker>::New())
    , m_cellPicker(vtkSmartPointer<vtkCellPicker>::New())
    , m_pickedIndex(-1)
    , m_pickedIndexType(IndexType::points)
    , m_pickedDataObject(nullptr)
    , m_pickedVisualizedData(nullptr)
{
}

Picker::~Picker() = default;

void Picker::pick(const vtkVector2i & clickPosXY, vtkRenderer & renderer)
{
    m_pickedIndex = -1;
    m_pickedDataObject = nullptr;
    m_pickedVisualizedData = nullptr;

    // pick points first; if this picker does not hit, we will also not hit cells
    m_pointPicker->Pick(clickPosXY[0], clickPosXY[1], 0, &renderer);

    auto pointMapper = m_pointPicker->GetMapper();
    if (!pointMapper)
    {
        return;
    }

    m_pickedVisualizedData = AbstractVisualizedData::readPointer(*pointMapper->GetInformation());
    m_pickedDataObject = &m_pickedVisualizedData->dataObject();

    if (!m_pickedVisualizedData)
    {
        qDebug() << "no visualization referenced in mapper";
        return;
    }

    QString content;
    QTextStream stream;
    stream.setString(&content);

    stream.setRealNumberNotation(QTextStream::RealNumberNotation::ScientificNotation);
    stream.setRealNumberPrecision(17);

    auto * inputInfo = pointMapper->GetInformation();

    QString inputName;
    if (inputInfo->Has(DataObject::NameKey()))
        inputName = DataObject::NameKey()->Get(inputInfo);
    else
        inputName = "(unnamed)";

    stream
        << "Data Set: " << inputName << endl;


    // ----------------------------
    // object type specific picking
    // ----------------------------

    m_cellPicker->Pick(clickPosXY[0], clickPosXY[1], 0, &renderer);
    auto cellMapper = m_cellPicker->GetMapper();

    if (cellMapper)
    {
        auto poly = dynamic_cast<PolyDataObject *>(m_pickedDataObject);
        if (poly)
        {
            appendPolyDataInfo(stream, *poly);
        }
    }
    else
    {
        auto image = dynamic_cast<ImageDataObject *>(m_pickedDataObject);
        if (image)
        {
            appendImageDataInfo(stream);
        }
        else
        {
            appendGlyphInfo(stream);
        }
    }


    QStringList info;
    QString line;

    while (stream.readLineInto(&line))
        info.push_back(line);

    m_pickedObjectInfo = info;

}

QStringList Picker::pickedObjectInfo()
{
    return m_pickedObjectInfo;
}

vtkIdType Picker::pickedIndex() const
{
    return m_pickedIndex;
}

IndexType Picker::pickedIndexType() const
{
    return m_pickedIndexType;
}

DataObject * Picker::pickedDataObject() const
{
    return m_pickedDataObject;
}

AbstractVisualizedData * Picker::pickedVisualizedData() const
{
    return m_pickedVisualizedData;
}

void Picker::appendPolyDataInfo(QTextStream & stream, PolyDataObject & polyData)
{
    auto cellMapper = m_cellPicker->GetMapper();
    m_pickedIndex = m_cellPicker->GetCellId();
    m_pickedIndexType = IndexType::cells;

    if (m_pickedIndex == -1)
    {
        return;
    }

    assert(cellMapper);

    double centroid[3];
    polyData.cellCenters()->GetPoint(m_pickedIndex, centroid);
    stream
        << "Triangle Index: " << m_pickedIndex << endl
        << "X = " << centroid[0] << endl
        << "Y = " << centroid[1] << endl
        << "Z = " << centroid[2];

    auto concreteMapper = vtkMapper::SafeDownCast(cellMapper);
    assert(concreteMapper);
    auto arrayName = concreteMapper->GetArrayName();
    auto  scalars = arrayName ? polyData.processedDataSet()->GetCellData()->GetArray(arrayName) : nullptr;
    if (scalars)
    {
        auto component = concreteMapper->GetLookupTable()->GetVectorComponent();
        assert(component >= 0);
        double value =
            scalars->GetTuple(m_pickedIndex)[component];

        stream << endl << endl << "Attribute: " << QString::fromUtf8(arrayName) << " (" << +component << ")" << endl;
        stream << "Value: " << value;
    }
}

void Picker::appendImageDataInfo(QTextStream & stream)
{
    auto pointMaper = m_pointPicker->GetMapper();
    m_pickedIndex = m_pointPicker->GetPointId();
    m_pickedIndexType = IndexType::points;

    assert(pointMaper);

    double * pos = m_pointPicker->GetPickPosition();

    auto dataSet = m_pointPicker->GetDataSet();
    assert(pointMaper->GetInputDataObject(0, 0) == dataSet);

    stream
        << "X = : " << pos[0] << endl
        << "Y = : " << pos[1] << endl;

    auto concreteMapper = vtkMapper::SafeDownCast(pointMaper);
    assert(concreteMapper);
    auto arrayName = concreteMapper->GetArrayName();
    auto  scalars = arrayName ? dataSet->GetPointData()->GetArray(arrayName) : nullptr;
    if (scalars)
    {
        auto component = concreteMapper->GetLookupTable()->GetVectorComponent();
        assert(component >= 0);
        double value =
            scalars->GetTuple(m_pickedIndex)[component];

        stream << endl << endl << "Attribute: " << QString::fromUtf8(arrayName) << " (" << +component << ")" << endl;
        stream << "Value: " << value;
    }
}

void Picker::appendGlyphInfo(QTextStream & stream)
{
    m_pickedIndex = m_pointPicker->GetPointId();
    m_pickedIndexType = IndexType::points;

    double* pos = m_pointPicker->GetPickPosition();
    stream
        << "Point Index: " << m_pickedIndex << endl
        << "X = " << pos[0] << endl
        << "Y = " << pos[1] << endl
        << "Z = " << pos[2];
}
