#include "Picker.h"

#include <cassert>

#include <QDebug>

#include <vtkActor.h>
#include <vtkCellData.h>
#include <vtkCellPicker.h>
#include <vtkImageSlice.h>
#include <vtkImageMapper3D.h>
#include <vtkImageProperty.h>
#include <vtkInformation.h>
#include <vtkInformationStringKey.h>
#include <vtkMapper.h>
#include <vtkPointData.h>
#include <vtkPointPicker.h>
#include <vtkPolyData.h>
#include <vtkPropCollection.h>
#include <vtkPropPicker.h>
#include <vtkScalarsToColors.h>
#include <vtkVector.h>

#include <core/types.h>
#include <core/data_objects/ImageDataObject.h>
#include <core/data_objects/PolyDataObject.h>
#include <core/rendered_data/RenderedData.h>


Picker::Picker()
    : m_propPicker(vtkSmartPointer<vtkPropPicker>::New())
    , m_cellPicker(vtkSmartPointer<vtkCellPicker>::New())
    , m_pointPicker(vtkSmartPointer<vtkPointPicker>::New())
    , m_pickedIndex(-1)
    , m_pickedIndexType(IndexType::points)
    , m_pickedDataObject(nullptr)
    , m_pickedVisualizedData(nullptr)
{
    m_cellPicker->PickFromListOn();
}

Picker::~Picker() = default;

void Picker::pick(const vtkVector2i & clickPosXY, vtkRenderer & renderer)
{
    m_pickedObjectInfo.clear();
    m_pickedIndex = -1;
    m_pickedDataObject = nullptr;
    m_pickedVisualizedData = nullptr;

    // pick points first; if this picker does not hit, we will also not hit cells
    m_propPicker->Pick(clickPosXY[0], clickPosXY[1], 0, &renderer);

    auto prop3D = m_propPicker->GetProp3D();
    if (!prop3D)
    {
        return;
    }

    vtkAbstractMapper * abstractMapper = nullptr;
    vtkActor * actor = nullptr;
    vtkImageSlice * imageSlice = nullptr;

    actor = m_propPicker->GetActor();

    if (actor)
    {
        abstractMapper = actor->GetMapper();
    }
    else
    {
        imageSlice = vtkImageSlice::SafeDownCast(prop3D);
        assert(imageSlice);
        if (imageSlice)
        {
            abstractMapper = imageSlice->GetMapper();
        }
    }

    if (!abstractMapper)
    {
        qDebug() << "Unknown object picked at: " + QString::number(clickPosXY[0]) + " " + QString::number(clickPosXY[1]);
        return;
    }

    auto mapperInfo = abstractMapper->GetInformation();

    m_pickedVisualizedData = AbstractVisualizedData::readPointer(*mapperInfo);
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
    stream.setRealNumberPrecision(5);

    QString inputName;
    if (mapperInfo->Has(DataObject::NameKey()))
        inputName = DataObject::NameKey()->Get(mapperInfo);
    else
        inputName = "(unnamed)";

    stream
        << "Data Set: " << inputName << endl;


    // ----------------------------
    // object type specific picking
    // ----------------------------

    if (auto poly = dynamic_cast<PolyDataObject *>(m_pickedDataObject))
    {
        assert(dynamic_cast<RenderedData *>(m_pickedVisualizedData));
        auto renderedData = static_cast<RenderedData *>(m_pickedVisualizedData);

        // let the cell picker only see the current object, to work around different precisions of vtkPropPicker and vtkCellPicker
        m_cellPicker->GetPickList()->RemoveAllItems();
        auto && viewProps = renderedData->viewProps();

        for (viewProps->InitTraversal(); auto prop = viewProps->GetNextProp();)
        {
            m_cellPicker->GetPickList()->AddItem(prop);
        }

        m_cellPicker->Pick(clickPosXY[0], clickPosXY[1], 0, &renderer);
        appendPolyDataInfo(stream, *poly);
    }
    else
    {
        m_pointPicker->Pick(clickPosXY[0], clickPosXY[1], 0, &renderer);

        if (imageSlice)
        {
            appendImageDataInfo(stream, *imageSlice);
        }
        else
        {
            appendGlyphInfo(stream);
        }
    }

    m_pickedObjectInfo = stream.readAll();
}

const QString & Picker::pickedObjectInfo() const
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

void Picker::appendImageDataInfo(QTextStream & stream, vtkImageSlice & slice)
{
    m_pickedIndex = m_pointPicker->GetPointId();
    m_pickedIndexType = IndexType::points;

    double * pos = m_pointPicker->GetPickPosition();

    auto dataSet = m_pointPicker->GetDataSet();

    stream
        << "X = : " << pos[0] << endl
        << "Y = : " << pos[1] << endl;

    auto scalars = dataSet->GetPointData()->GetScalars();

    if (scalars)
    {
        const auto tuple = scalars->GetTuple(m_pickedIndex);
        if (auto lut = slice.GetProperty()->GetLookupTable())
        {
            auto component = lut->GetVectorComponent();
            double value = tuple[component];
            stream << endl << "Attribute component: " << component << endl;
            stream << "Value: " << value;
        }
        else if (scalars->GetNumberOfComponents() >= 3)
        {
            stream << endl << "Color" << endl;
            stream << "R: " << tuple[0] << endl;
            stream << "G: " << tuple[1] << endl;
            stream << "B: " << tuple[2] << endl;
            if (scalars->GetNumberOfComponents() >= 4)
            {
                stream << "A: " << tuple[1];
            }
        }
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
