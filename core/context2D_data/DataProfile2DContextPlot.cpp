#include "DataProfile2DContextPlot.h"

#include <cassert>
#include <vector>

#include <vtkAssignAttribute.h>
#include <vtkCellData.h>
#include <vtkDataSet.h>
#include <vtkFloatArray.h>
#include <vtkPen.h>
#include <vtkPlotLine.h>
#include <vtkPointData.h>
#include <vtkTable.h>

#include <reflectionzeug/PropertyGroup.h>

#include <core/data_objects/DataProfile2DDataObject.h>
#include <core/context2D_data/vtkPlotCollection.h>
#include <core/utility/DataExtent.h>


using namespace reflectionzeug;


DataProfile2DContextPlot::DataProfile2DContextPlot(DataProfile2DDataObject & dataObject)
    : Context2DData(dataObject)
    , m_plotLine{ vtkSmartPointer<vtkPlotLine>::New() }
    , m_title{ dataObject.scalarsName() }
{
    connect(&dataObject, &DataObject::dataChanged, this, &DataProfile2DContextPlot::updatePlot);
}

DataProfile2DContextPlot::~DataProfile2DContextPlot() = default;

std::unique_ptr<PropertyGroup> DataProfile2DContextPlot::createConfigGroup()
{
    auto root = std::make_unique<PropertyGroup>();

    root->addProperty<std::string>("Title",
        [this] () { return title().toStdString(); },
        [this] (const std::string & s) {
        setTitle(QString::fromStdString(s));
        geometryChanged();
    });

    root->addProperty<Color>("Color",
        [this] () {
        unsigned char rgb[3];
        m_plotLine->GetColor(rgb);
        return Color(rgb[0], rgb[1], rgb[2], m_plotLine->GetPen()->GetOpacity());
    },
        [this] (const Color & color) {
        m_plotLine->SetColor((unsigned char)(color.red()), (unsigned char)(color.green()), (unsigned char)(color.blue()), (unsigned char)(color.alpha()));
        emit geometryChanged();
    });

    auto prop_transparency = root->addProperty<unsigned char>("Transparency",
        [this] () {
        return static_cast<unsigned char>((1.0f - m_plotLine->GetPen()->GetOpacity() / 255.f) * 100);
    },
        [this] (unsigned char transparency) {
        m_plotLine->GetPen()->SetOpacity(static_cast<unsigned char>(
            (100.f - transparency) / 100.f * 255.f));
        emit geometryChanged();
    });
    prop_transparency->setOption("minimum", 0);
    prop_transparency->setOption("maximum", 100);
    prop_transparency->setOption("step", 1);
    prop_transparency->setOption("suffix", " %");

    auto prop_width = root->addProperty<float>("Width",
        [this] () { return m_plotLine->GetWidth(); },
        [this] (float width) {
        m_plotLine->SetWidth(width);
        emit geometryChanged();
    });
    prop_width->setOption("minimum", 0.001f);

    return root;
}

DataProfile2DDataObject & DataProfile2DContextPlot::profileData()
{
    return static_cast<DataProfile2DDataObject &>(dataObject());
}

const DataProfile2DDataObject & DataProfile2DContextPlot::profileData() const
{
    return static_cast<const DataProfile2DDataObject &>(dataObject());
}

const QString & DataProfile2DContextPlot::title() const
{
    return m_title;
}

void DataProfile2DContextPlot::setTitle(const QString & title)
{
    if (title == m_title)
    {
        return;
    }

    m_title = title;

    updatePlot();
}

vtkSmartPointer<vtkPlotCollection> DataProfile2DContextPlot::fetchPlots()
{
    auto items = vtkSmartPointer<vtkPlotCollection>::New();

    updatePlot();

    items->AddItem(m_plotLine);

    return items;
}

DataBounds DataProfile2DContextPlot::updateVisibleBounds()
{
    auto table = m_plotLine->GetInput();
    assert(table);
    auto xAxis = vtkDataArray::SafeDownCast(table->GetColumn(0));
    auto yAxis = vtkDataArray::SafeDownCast(table->GetColumn(1));
    const vtkIdType numValues = xAxis->GetNumberOfTuples();
    assert(xAxis && yAxis && (numValues == yAxis->GetNumberOfTuples()));

    ValueRange<> xRange, yRange;
    xAxis->GetRange(xRange.data());
    yAxis->GetRange(yRange.data());

    return DataBounds({ xRange, yRange, ValueRange<>() });
}

void DataProfile2DContextPlot::updatePlot()
{
    auto profilePoints = profileData().processedDataSet();
    const vtkIdType numPoints = profilePoints->GetNumberOfPoints();

    if (numPoints < 2)  // produces a warning
    {
        setPlotIsValid(false);
        return;
    }

    auto sourceYValues = profilePoints->GetPointData()->GetArray(profileData().scalarsName().toUtf8().data());
    assert(sourceYValues && sourceYValues->GetNumberOfTuples() == numPoints);

    auto plotYValues = vtkSmartPointer<vtkDataArray>::Take(sourceYValues->NewInstance());
    if (sourceYValues->GetNumberOfComponents() == 1)
    {
        plotYValues->DeepCopy(sourceYValues);
    }
    else
    {
        const auto component = static_cast<vtkIdType>(profileData().vectorComponent());
        assert(sourceYValues->GetNumberOfComponents() > component);
        auto tuple = std::vector<double>(sourceYValues->GetNumberOfComponents());

        plotYValues->SetNumberOfTuples(numPoints);
        for (vtkIdType i = 0; i < numPoints; ++i)
        {
            sourceYValues->GetTuple(i, tuple.data());
            plotYValues->SetTuple(i, std::next(tuple.data(), component));
        }
    }

    plotYValues->SetName(m_title.toUtf8().data());

    auto table = vtkSmartPointer<vtkTable>::New();
    auto xAxis = vtkSmartPointer<vtkFloatArray>::New();
    xAxis->SetNumberOfValues(numPoints);
    xAxis->SetName(profileData().abscissa().toUtf8().data());
    table->AddColumn(xAxis);
    table->AddColumn(plotYValues);

    for (int i = 0; i < profilePoints->GetNumberOfPoints(); ++i)
    {
        double point[3];
        profilePoints->GetPoint(i, point);
        table->SetValue(i, 0, point[0]);
    }

    m_plotLine->SetInputData(table, 0, 1);

    setPlotIsValid(true);
    invalidateVisibleBounds();
}

void DataProfile2DContextPlot::setPlotIsValid(bool isValid)
{
    m_plotLine->SetVisible(isValid);
}
