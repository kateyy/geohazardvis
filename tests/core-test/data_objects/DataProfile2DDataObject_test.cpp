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

#include <gtest/gtest.h>

#include <algorithm>
#include <numeric>
#include <vector>

#include <vtkAlgorithm.h>
#include <vtkAlgorithmOutput.h>
#include <vtkCellArray.h>
#include <vtkExecutive.h>
#include <vtkFloatArray.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>
#include <vtkVector.h>

#include <core/data_objects/DataProfile2DDataObject.h>
#include <core/data_objects/PointCloudDataObject.h>
#include <core/utility/vtkvectorhelper.h>


class DataProfile2DDataObject_test : public ::testing::Test
{
public:
    vtkSmartPointer<vtkPolyData> genPointPolyData(
        const std::vector<vtkVector3d> & coords,
        const vtkVector3d & offset = vtkVector3d{ 0.0, 0.0, 0.0 })
    {
        auto poly = vtkSmartPointer<vtkPolyData>::New();
        auto points = vtkSmartPointer<vtkPoints>::New();
        points->SetNumberOfPoints(static_cast<vtkIdType>(coords.size()));
        for (vtkIdType i = 0; i < static_cast<vtkIdType>(coords.size()); ++i)
        {
            points->SetPoint(i, (offset + coords[i]).GetData());
        }
        poly->SetPoints(points);
        std::vector<vtkIdType> pointIds(coords.size());
        std::iota(pointIds.begin(), pointIds.end(), 0);
        auto vertices = vtkSmartPointer<vtkCellArray>::New();
        vertices->InsertNextCell(static_cast<vtkIdType>(coords.size()), pointIds.data());
        poly->SetVerts(vertices);
        return poly;
    }
};


TEST_F(DataProfile2DDataObject_test, PlotRegularPointCloud)
{
    // 3x3 points regularly placed in a square
    auto pointPolyData = genPointPolyData({
        { 1.0, 1.0, 0.0 },
        { 2.0, 1.0, 0.0 },
        { 3.0, 1.0, 0.0 },
        { 1.0, 2.0, 0.0 },
        { 2.0, 2.0, 0.0 },
        { 3.0, 2.0, 0.0 },
        { 1.0, 3.0, 0.0 },
        { 2.0, 3.0, 0.0 },
        { 3.0, 3.0, 0.0 } });
    // Mark diagonal points, which are the points that should be included in the plot
    auto scalars = vtkSmartPointer<vtkFloatArray>::New();
    scalars->SetName("Diagonal");
    scalars->SetNumberOfValues(9);
    scalars->Fill(-1.0);
    scalars->SetValue(0, 1.f);
    scalars->SetValue(4, 2.f);
    scalars->SetValue(8, 3.f);
    pointPolyData->GetPointData()->SetScalars(scalars);

    PointCloudDataObject pointCloud("Point Cloud", *pointPolyData);
    DataProfile2DDataObject plot("Plot", pointCloud, "Diagonal", IndexType::points, 0);
    ASSERT_TRUE(plot.isValid());
    plot.setProfileLinePoints({ 1.0, 1.0 }, { 3.0, 3.0 });
    auto output = plot.processedOutputPort()->GetProducer();
    ASSERT_TRUE(output->GetExecutive()->Update());
    auto outPoly = vtkPolyData::SafeDownCast(output->GetOutputDataObject(0));
    ASSERT_TRUE(outPoly);
    ASSERT_EQ(3, outPoly->GetNumberOfPoints());
    auto plotScalars = outPoly->GetPointData()->GetScalars();
    ASSERT_TRUE(plotScalars);
    ASSERT_EQ(1., plotScalars->GetComponent(0, 0));
    ASSERT_EQ(2., plotScalars->GetComponent(1, 0));
    ASSERT_EQ(3., plotScalars->GetComponent(2, 0));
}

TEST_F(DataProfile2DDataObject_test, PlotIrregularPointCloud)
{
    // Target points: near line (1.1), (20, 5)
    auto pointPolyData = genPointPolyData({
        { 1.0, 1.0, 0.0 },  // +
        { 2.0, 1.0, 0.0 },  // +
        { 5.0, 5.0, 0.0 },  // -
        { 9.0, 4.0, 0.0 },  // +
        { 15.0, 1.0, 0.0 }, // -
        { 15.0, 2.0, 0.0 }, // +
        { 20.0, 5.0, 0.0 } });  // +
    // Mark points that should be included in the plot
    auto scalars = vtkSmartPointer<vtkFloatArray>::New();
    scalars->SetName("OnLine");
    scalars->SetNumberOfValues(7);
    scalars->Fill(-1.0);
    scalars->SetValue(0, 1.f);
    scalars->SetValue(1, 2.f);
    scalars->SetValue(3, 3.f);
    scalars->SetValue(5, 4.f);
    scalars->SetValue(6, 5.f);
    pointPolyData->GetPointData()->SetScalars(scalars);

    PointCloudDataObject pointCloud("Point Cloud", *pointPolyData);
    DataProfile2DDataObject plot("Plot", pointCloud, "OnLine", IndexType::points, 0);
    ASSERT_TRUE(plot.isValid());
    plot.setProfileLinePoints({ 1.0, 1.0 }, { 20.0, 5.0 });
    auto output = plot.processedOutputPort()->GetProducer();
    ASSERT_TRUE(output->GetExecutive()->Update());
    auto outPoly = vtkPolyData::SafeDownCast(output->GetOutputDataObject(0));
    ASSERT_TRUE(outPoly);
    ASSERT_EQ(5, outPoly->GetNumberOfPoints());
    auto plotScalars = outPoly->GetPointData()->GetScalars();
    ASSERT_TRUE(plotScalars);
    for (vtkIdType i = 0; i < 5; ++i)
    {
        ASSERT_EQ(double(i + 1), plotScalars->GetComponent(i, 0));
    }
}

TEST_F(DataProfile2DDataObject_test, PlotIrregularPointCloudFromGlobalMetric)
{
    const vtkVector2d globalMetricEastingNorthing_km{ 481.50047, 4553.50582 };
    const vtkVector2d globalLatLong{ 41.132648, 14.779591 };
    // Target points: near line (15,2), (23,7)
    auto pointPolyData = genPointPolyData({
        { 0.0, 0.0, 0.0 },  // -
        { 1.0, 1.0, 0.0 },  // -
        { 2.0, 1.0, 0.0 },  // -
        { 5.0, 5.0, 0.0 },  // -
        { 9.0, 4.0, 0.0 },  // -
        { 15.0, 1.0, 0.0 }, // -
        { 15.0, 2.0, 0.0 }, // +
        { 20.0, 5.0, 0.0 },  // +
        { 23.0, 7.0, 0.0 },  // +
        { 24.0, 7.0, 0.0 },  // -
    },
        convertTo<3>(globalMetricEastingNorthing_km, 0.0));    // shift to the "global metric" coordinates
    // Mark points that should be included in the plot
    auto scalars = vtkSmartPointer<vtkFloatArray>::New();
    scalars->SetName("OnLine");
    scalars->SetNumberOfValues(pointPolyData->GetNumberOfPoints());
    scalars->Fill(-1.0);
    scalars->SetValue(6, 1.f);
    scalars->SetValue(7, 2.f);
    scalars->SetValue(8, 3.f);
    pointPolyData->GetPointData()->SetScalars(scalars);
    PointCloudDataObject pointCloud("Point Cloud", *pointPolyData);
    const auto coordsSpec = ReferencedCoordinateSystemSpecification(
        CoordinateSystemType::metricGlobal, "WGS 84", "UTM", "km", globalLatLong);
    pointCloud.specifyCoordinateSystem(coordsSpec);

    DataProfile2DDataObject plot("Plot", pointCloud, "OnLine", IndexType::points, 0);
    ASSERT_TRUE(plot.isValid());
    plot.setPointsCoordinateSystem(coordsSpec);
    plot.setProfileLinePoints(
        globalMetricEastingNorthing_km + vtkVector2d{ 15.0, 2.0 },
        globalMetricEastingNorthing_km + vtkVector2d{ 23.0, 7.0 });
    auto output = plot.processedOutputPort()->GetProducer();
    ASSERT_TRUE(output->GetExecutive()->Update());
    auto outPoly = vtkPolyData::SafeDownCast(output->GetOutputDataObject(0));
    ASSERT_TRUE(outPoly);
    ASSERT_EQ(3, outPoly->GetNumberOfPoints());
    auto plotScalars = outPoly->GetPointData()->GetScalars();
    ASSERT_TRUE(plotScalars);
    for (vtkIdType i = 0; i < 3; ++i)
    {
        ASSERT_EQ(double(i + 1), plotScalars->GetComponent(i, 0));
    }
}
