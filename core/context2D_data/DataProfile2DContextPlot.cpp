/*
 * GeohazardVis
 * Copyright (C) 2017 Karsten Tausche <geodev@posteo.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

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
#include <vtkPointSet.h>
#include <vtkTable.h>

#include <reflectionzeug/PropertyGroup.h>

#include <core/OpenGLDriverFeatures.h>
#include <core/context2D_data/vtkPlotCollection.h>
#include <core/data_objects/DataProfile2DDataObject.h>
#include <core/reflectionzeug_extension/QStringProperty.h>
#include <core/utility/DataExtent.h>


using namespace reflectionzeug;


DataProfile2DContextPlot::DataProfile2DContextPlot(DataProfile2DDataObject & dataObject)
    : Context2DData(dataObject)
    , m_plotLine{ vtkSmartPointer<vtkPlotLine>::New() }
    , m_title{ dataObject.scalarsName() }
{
    connect(&dataObject, &DataObject::dataChanged,
        this, &DataProfile2DContextPlot::updatePlotRedraw);
    connect(&dataObject, &DataProfile2DDataObject::sourceDataChanged,
        this, &DataProfile2DContextPlot::updatePlotRedraw);
}

DataProfile2DContextPlot::~DataProfile2DContextPlot() = default;

std::unique_ptr<PropertyGroup> DataProfile2DContextPlot::createConfigGroup()
{
    auto root = std::make_unique<PropertyGroup>();

    root->addProperty<QString>("Title",
        [this] () { return title(); },
        [this] (const QString & s) {
        setTitle(s);
        geometryChanged();
    });

    root->addProperty<Color>("Color",
        [this] () {
        const auto rgb = color();
        return Color(rgb[0], rgb[1], rgb[2], m_plotLine->GetPen()->GetOpacity());
    },
        [this] (const Color & color) {
        setColor(vtkColor3ub((unsigned char)(color.red()), (unsigned char)(color.green()), (unsigned char)(color.blue())));
        m_plotLine->GetPen()->SetOpacity((unsigned char)color.alpha());
        emit geometryChanged();
    });

    root->addProperty<unsigned char>("Transparency",
        [this] () {
        return static_cast<unsigned char>((1.0f - m_plotLine->GetPen()->GetOpacity() / 255.f) * 100);
    },
        [this] (unsigned char transparency) {
        m_plotLine->GetPen()->SetOpacity(static_cast<unsigned char>(
            (100.f - transparency) / 100.f * 255.f));
        emit geometryChanged();
    })->setOptions({
        { "minimum", 0 },
        { "maximum", 100 },
        { "step", 1 },
        { "suffix", " %" }
    });

    root->addProperty<float>("Width",
        [this] () { return m_plotLine->GetWidth(); },
        [this] (float width) {
        m_plotLine->SetWidth(width);
        emit geometryChanged();
    })->setOptions({
        { "minimum", 1.f },
        { "maximum", OpenGLDriverFeatures::clampToMaxSupportedLineWidth(100.f) }
    });

    return root;
}

void DataProfile2DContextPlot::setColor(const vtkColor3ub & color)
{
    m_plotLine->SetColor(color[0], color[1], color[2], 0xFF);
}

vtkColor3ub DataProfile2DContextPlot::color() const
{
    vtkColor3ub c;
    m_plotLine->GetColor(c.GetData());
    return c;
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
    auto xAxis = vtkDataArray::FastDownCast(table->GetColumn(0));
    auto yAxis = vtkDataArray::FastDownCast(table->GetColumn(1));
    assert(xAxis && yAxis && (xAxis->GetNumberOfTuples() == yAxis->GetNumberOfTuples()));

    ValueRange<> xRange, yRange;
    xAxis->GetRange(xRange.data());
    yAxis->GetRange(yRange.data());

    return DataBounds({ xRange, yRange, ValueRange<>() });
}

void DataProfile2DContextPlot::updatePlot()
{
    auto profileDataSet = vtkPointSet::SafeDownCast(profileData().processedOutputDataSet());
    if (!profileDataSet)
    {
        setPlotIsValid(false);
        return;
    }

    const vtkIdType numPoints = profileDataSet->GetNumberOfPoints();

    if (numPoints < 2)  // produces a warning
    {
        setPlotIsValid(false);
        return;
    }

    auto sourceYValues = profileDataSet->GetPointData()->GetArray(profileData().scalarsName().toUtf8().data());
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
    auto profilePoints = profileDataSet->GetPoints()->GetData();
    for (vtkIdType i = 0; i < numPoints; ++i)
    {
        xAxis->SetValue(i, static_cast<float>(profilePoints->GetComponent(i, 0)));
    }

    table->AddColumn(xAxis);
    table->AddColumn(plotYValues);

    m_plotLine->SetInputData(table, 0, 1);

    setPlotIsValid(true);
    invalidateVisibleBounds();
}

void DataProfile2DContextPlot::updatePlotRedraw()
{
    updatePlot();
    emit geometryChanged();
}

void DataProfile2DContextPlot::setPlotIsValid(bool isValid)
{
    m_plotLine->SetVisible(isValid);
}
