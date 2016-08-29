#include "Picker.h"

#include <array>
#include <cassert>
#include <vector>

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
#include <vtkVersionMacros.h>

#include <core/types.h>
#include <core/data_objects/ImageDataObject.h>
#include <core/data_objects/PolyDataObject.h>
#include <core/rendered_data/RenderedData.h>
#include <core/utility/macros.h>


Picker::Picker()
    : m_propPicker{ vtkSmartPointer<vtkPropPicker>::New() }
    , m_cellPicker{ vtkSmartPointer<vtkCellPicker>::New() }
    , m_pointPicker{ vtkSmartPointer<vtkPointPicker>::New() }
{
    m_cellPicker->PickFromListOn();
    m_pointPicker->PickFromListOn();
}

Picker::~Picker() = default;

void Picker::pick(const vtkVector2i & clickPosXY, vtkRenderer & renderer)
{
    m_pickedObjectInfoString.clear();
    m_pickedObjectInfo.clear();

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

    auto & mapperInfo = *abstractMapper->GetInformation();

    m_pickedObjectInfo.visualization = AbstractVisualizedData::readPointer(mapperInfo);
    if (!m_pickedObjectInfo.visualization)
    {
        qDebug() << "no visualization referenced in mapper";
        return;
    }

    // TODO determine correct visualization port
    m_pickedObjectInfo.visOutputPort = m_pickedObjectInfo.visualization->defaultVisualizationPort();

    QString content;
    QTextStream stream;
    stream.setString(&content);

    stream.setRealNumberNotation(QTextStream::RealNumberNotation::ScientificNotation);
    stream.setRealNumberPrecision(5);

    auto inputName = DataObject::readName(mapperInfo);
    if (inputName.isEmpty())
    {
        inputName = "(unnamed)";
    }

    stream << "Data Set: " << inputName << endl;


    // ----------------------------
    // object type specific picking
    // ----------------------------

    assert(dynamic_cast<RenderedData *>(m_pickedObjectInfo.visualization));
    auto renderedData = static_cast<RenderedData *>(m_pickedObjectInfo.visualization);

    if (auto poly = dynamic_cast<PolyDataObject *>(&m_pickedObjectInfo.visualization->dataObject()))
    {
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
    else // point based data sets: images, volumes, volume slices, glyphs
    {
        m_pointPicker->GetPickList()->RemoveAllItems();
        auto && viewProps = renderedData->viewProps();

        for (viewProps->InitTraversal(); auto prop = viewProps->GetNextProp();)
        {
            m_pointPicker->GetPickList()->AddItem(prop);
        }

        m_pointPicker->Pick(clickPosXY[0], clickPosXY[1], 0, &renderer);

        if (imageSlice)
        {
            appendImageDataInfo(stream, *imageSlice);
        }
        else
        {
            appendGenericPointInfo(stream);
        }
    }

    m_pickedObjectInfoString = stream.readAll();
}

const QString & Picker::pickedObjectInfoString() const
{
    return m_pickedObjectInfoString;
}

const VisualizationSelection & Picker::pickedObjectInfo() const
{
    return m_pickedObjectInfo;
}

void Picker::appendPolyDataInfo(QTextStream & stream, PolyDataObject & polyData)
{
    auto cellMapper = m_cellPicker->GetMapper();
    const auto pickedIndex = m_cellPicker->GetCellId();

    if (pickedIndex == -1)
    {
        return;
    }

    m_pickedObjectInfo.indexType = IndexType::cells;
    m_pickedObjectInfo.indices = { pickedIndex };

    assert(cellMapper);

    std::array<double, 3> centroid;
    polyData.cellCenters()->GetPoint(pickedIndex, centroid.data());
    stream
        << "Triangle Index: " << pickedIndex << endl
        << "X = " << centroid[0] << endl
        << "Y = " << centroid[1] << endl
        << "Z = " << centroid[2];

    auto concreteMapper = vtkMapper::SafeDownCast(cellMapper);
    assert(concreteMapper);
    if (!concreteMapper->GetScalarVisibility())
    {
        return;
    }

    const auto scalarMode = concreteMapper->GetScalarMode();
    auto sourceAttributes = [scalarMode, concreteMapper] () -> vtkDataSetAttributes *
    {
        switch (scalarMode)
        {
        case VTK_SCALAR_MODE_USE_CELL_DATA:
            return concreteMapper->GetInputAsDataSet()->GetCellData();
        case VTK_SCALAR_MODE_USE_POINT_DATA:
            return concreteMapper->GetInputAsDataSet()->GetPointData();
        default:
            return nullptr;
        }
    }();

    if (!sourceAttributes)
    {
        return;
    }

    auto scalars = sourceAttributes->GetArray(concreteMapper->GetArrayName());

    if (!scalars)
    {
        return;
    }

    printScalarInfo(stream, concreteMapper->GetLookupTable(), *scalars, pickedIndex);
}

void Picker::appendImageDataInfo(QTextStream & stream, vtkImageSlice & slice)
{
    const auto pickedIndex = m_pointPicker->GetPointId();
    auto dataSet = m_pointPicker->GetDataSet();

    if (pickedIndex == -1 || !dataSet)
    {
        return;
    }

    m_pickedObjectInfo.indexType = IndexType::points;
    m_pickedObjectInfo.indices = { pickedIndex };

    std::array<double, 3> pos;
    m_pointPicker->GetPickPosition(pos.data());

    stream
        << "X = : " << pos[0] << endl
        << "Y = : " << pos[1] << endl;

    if (auto scalars = dataSet->GetPointData()->GetScalars())
    {
        printScalarInfo(stream, slice.GetProperty()->GetLookupTable(), *scalars, pickedIndex);
    }
}

void Picker::appendGenericPointInfo(QTextStream & stream)
{
    std::array<double, 3> pos;
    m_pointPicker->GetPickPosition(pos.data());

    // TODO check how to match (resampled) glyph/volume slice indices with input data indices
    m_pickedObjectInfo.indexType = IndexType::invalid;
    m_pickedObjectInfo.indices.clear();

    stream
        << "X = " << pos[0] << endl
        << "Y = " << pos[1] << endl
        << "Z = " << pos[2];
}

void Picker::printScalarInfo(QTextStream & stream, vtkScalarsToColors * lut,
    vtkDataArray & scalars, vtkIdType pickedIndex)
{
    auto tuple = std::vector<double>(scalars.GetNumberOfComponents());
    scalars.GetTuple(pickedIndex, tuple.data());

    QString unitStr;
#if VTK_CHECK_VERSION(7, 1, 0)
    unitStr = scalars.GetInformation()->Get(vtkDataArray::UNITS_LABEL());
    if (!unitStr.isEmpty())
    {
        unitStr.prepend(" ");
    }
#endif

    if (lut)
    {
        const auto component = lut->GetVectorComponent();
        assert(component >= 0);
        const double value = tuple[component];

        stream << endl << endl << "Attribute: " << QString::fromUtf8(scalars.GetName());
        if (tuple.size() > 1)
        {
            stream << " (Component " << component + 1 << ")";
        }
        stream << endl << "Value: " << value << unitStr;
    }
    else if (tuple.size() == 3 || tuple.size() == 4)
    {
        stream << endl << "Color" << endl;
        stream << "R: " << tuple[0] << unitStr << endl;
        stream << "G: " << tuple[1] << unitStr << endl;
        stream << "B: " << tuple[2] << unitStr;
        if (tuple.size() == 4)
        {
            stream << endl << "A: " << tuple[3] << unitStr;
        }
    }
}
