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

#include <array>

#include <vtkExecutive.h>
#include <vtkFloatArray.h>
#include <vtkImageData.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkTriangle.h>

#include <core/CoordinateSystems.h>
#include <core/filters/GeographicTransformationFilter.h>
#include <core/utility/DataExtent.h>
#include <core/utility/vtkvectorhelper.h>

#include <core/utility/GeographicTransformationUtil.h>


class GeographicTransformationFilter_test : public ::testing::Test
{
public:
    static const DataBounds & dataBounds_WGS84()
    {
        static const auto bounds =
            DataBounds({ -64.702091, -64.684994, 32.354337, 32.359953, 200, 500 });

        return bounds;
    }
    static const DataBounds & dataBounds_WGS84_UTM()
    {
        static const auto bounds =
            DataBounds({ 339839.63, 341458.40, 3580985.71, 3581582.89, 200, 500 });
        return bounds;
    }
    static const DataBounds & dataBounds_WGS84_UTM_local()
    {
        static const auto bounds =
            DataBounds({ -809.41, 809.36, -298.56, 298.62, 200, 500 });
        return bounds;
    }
    static const ReferencedCoordinateSystemSpecification & dataSpec_WGS84()
    {
        static const auto & spec = ReferencedCoordinateSystemSpecification(
            CoordinateSystemType::geographic,
            "WGS 84",
            "UTM",
            {},
            { dataBounds_WGS84().center()[1], dataBounds_WGS84().center()[0] });
        return spec;
    }
    static const ReferencedCoordinateSystemSpecification & dataSpec_WGS84_UTM()
    {
        static const auto & spec = ReferencedCoordinateSystemSpecification(
            CoordinateSystemType::metricGlobal,
            "WGS 84",
            "UTM",
            "m",
            { dataBounds_WGS84().center()[1], dataBounds_WGS84().center()[0] });
        return spec;
    }
    static const ReferencedCoordinateSystemSpecification & dataSpec_WGS84_UTM_local()
    {
        static const auto & spec = ReferencedCoordinateSystemSpecification(
            CoordinateSystemType::metricLocal,
            "WGS 84",
            "UTM",
            "m",
            { dataBounds_WGS84().center()[1], dataBounds_WGS84().center()[0] });
        return spec;
    }

    static vtkSmartPointer<vtkImageData> generateDEM(const CoordinateSystemType type)
    {
        auto spec = type == CoordinateSystemType::geographic ? dataSpec_WGS84()
            : (type == CoordinateSystemType::metricGlobal ? dataSpec_WGS84_UTM()
                : dataSpec_WGS84_UTM_local());
        auto bounds = type == CoordinateSystemType::geographic
            ? dataBounds_WGS84()
            : (type == CoordinateSystemType::metricGlobal ? dataBounds_WGS84_UTM()
                : dataBounds_WGS84_UTM_local());

        auto image = vtkSmartPointer<vtkImageData>::New();
        image->SetExtent(0, 10, 0, 10, 0, 0);
        image->SetOrigin(bounds.min().GetData());
        auto spacing = bounds.componentSize() / 10.0;
        for (int i = 0; i < 3; ++i)
        {
            spacing[i] = spacing[i] == 0 ? 1.0 : spacing[i];
        }
        image->SetSpacing(spacing.GetData());

        spec.writeToFieldData(*image->GetFieldData());

        auto elevations = vtkSmartPointer<vtkFloatArray>::New();
        elevations->SetNumberOfValues(image->GetNumberOfPoints());
        elevations->SetName("elevations");
        for (int i = 0; i < elevations->GetNumberOfValues(); ++i)
        {
            elevations->SetValue(i, static_cast<float>(i / (elevations->GetNumberOfValues() - 1)));
        }
        image->GetPointData()->SetScalars(elevations);

        return image;
    }

    static vtkSmartPointer<vtkPolyData> generatePoly(const CoordinateSystemType type)
    {
        auto spec = type == CoordinateSystemType::geographic ? dataSpec_WGS84()
            : (type == CoordinateSystemType::metricGlobal ? dataSpec_WGS84_UTM()
                : dataSpec_WGS84_UTM_local());
        auto bounds = type == CoordinateSystemType::geographic
            ? dataBounds_WGS84()
            : (type == CoordinateSystemType::metricGlobal ? dataBounds_WGS84_UTM()
                : dataBounds_WGS84_UTM_local());

        const auto min = bounds.min();
        const auto max = bounds.max();
        const auto center = bounds.center();

        auto poly = vtkSmartPointer<vtkPolyData>::New();
        auto points = vtkSmartPointer<vtkPoints>::New();
        points->SetDataTypeToDouble();
        /*0*/ points->InsertNextPoint(min.GetData());
        /*1*/ points->InsertNextPoint(max.GetData());
        /*2*/ points->InsertNextPoint(center[0], center[1], max[2]);
        auto tris = vtkSmartPointer<vtkCellArray>::New();
        std::array<vtkIdType, 3> ids;
        ids = { 0, 1, 2 }; tris->InsertNextCell(3, ids.data());

        poly->SetPoints(points);
        poly->SetPolys(tris);
        spec.writeToFieldData(*poly->GetFieldData());

        return poly;
    }

};

TEST_F(GeographicTransformationFilter_test, ImageGeoToLocal)
{
    auto filter = vtkSmartPointer<GeographicTransformationFilter>::New();
    auto dem = generateDEM(CoordinateSystemType::geographic);

    filter->SetInputData(dem);
    auto targetSpec = dataSpec_WGS84_UTM_local();
    targetSpec.unitOfMeasurement = "km";
    filter->SetTargetCoordinateSystem(targetSpec);
    ASSERT_TRUE(filter->GetExecutive()->Update());
    auto localDem = filter->GetOutput();
    ASSERT_TRUE(localDem);

    DataBounds bounds;
    localDem->GetBounds(bounds.data());
    auto expectedLocal =
        dataBounds_WGS84_UTM().shifted(-dataBounds_WGS84_UTM().center()).scaled(vtkVector3d(0.001));
    const double baseElevation = dataBounds_WGS84_UTM().extractDimension(2)[0];
    expectedLocal.setDimension(2, baseElevation, baseElevation);

    // Error of less than 5cm -> probably enough?
    // As an alternative, "geographiclib" promises for less than 2cm
    // https://sourceforge.net/projects/geographiclib/
    for (unsigned i = 0; i < 6; ++i)
    {
        ASSERT_NEAR(expectedLocal[i], bounds[i], 5.e-5);
    }
}

TEST_F(GeographicTransformationFilter_test, ImageLocalToGeo)
{
    auto filter = vtkSmartPointer<GeographicTransformationFilter>::New();
    auto dem = generateDEM(CoordinateSystemType::metricLocal);

    filter->SetInputData(dem);
    filter->SetTargetCoordinateSystem(dataSpec_WGS84());
    ASSERT_TRUE(filter->GetExecutive()->Update());
    auto geoDem = filter->GetOutput();
    ASSERT_TRUE(geoDem);

    DataBounds bounds;
    geoDem->GetBounds(bounds.data());
    auto expected = dataBounds_WGS84();
    expected.dimension(2)[1] = expected.dimension(2)[0];

    for (unsigned i = 0; i < 6; ++i)
    {
        ASSERT_NEAR(expected[i], bounds[i], 5.e-7);
    }
}

TEST_F(GeographicTransformationFilter_test, ImageLocalToGlobal)
{
    auto filter = vtkSmartPointer<GeographicTransformationFilter>::New();
    auto dem = generateDEM(CoordinateSystemType::metricLocal);

    filter->SetInputData(dem);
    auto targetSpec = dataSpec_WGS84_UTM();
    targetSpec.unitOfMeasurement = "km";
    filter->SetTargetCoordinateSystem(targetSpec);
    ASSERT_TRUE(filter->GetExecutive()->Update());
    auto geoDem = filter->GetOutput();
    ASSERT_TRUE(geoDem);

    DataBounds bounds;
    geoDem->GetBounds(bounds.data());
    auto expected = dataBounds_WGS84_UTM().scaled(vtkVector3d(0.001));
    expected.dimension(2)[1] = expected.dimension(2)[0];

    for (unsigned i = 0; i < 6; ++i)
    {
        ASSERT_NEAR(expected[i], bounds[i], 5.e-6);
    }
}

TEST_F(GeographicTransformationFilter_test, PolyGeoToLocal)
{
    auto filter = vtkSmartPointer<GeographicTransformationFilter>::New();
    auto poly = generatePoly(CoordinateSystemType::geographic);

    filter->SetInputData(poly);
    auto targetSpec = dataSpec_WGS84_UTM_local();
    targetSpec.unitOfMeasurement = "km";
    filter->SetTargetCoordinateSystem(targetSpec);
    ASSERT_TRUE(filter->GetExecutive()->Update());
    auto localDem = filter->GetOutput();
    ASSERT_TRUE(localDem);

    DataBounds bounds;
    localDem->GetBounds(bounds.data());
    auto expectedLocal = dataBounds_WGS84_UTM_local().scaled(vtkVector3d(0.001));
    expectedLocal.setDimension(2, dataBounds_WGS84().extractDimension(2));

    for (unsigned i = 0; i < 6; ++i)
    {
        ASSERT_NEAR(expectedLocal[i], bounds[i], 5.e-6);
    }
}

TEST_F(GeographicTransformationFilter_test, PolyLocalToGeo)
{
    auto filter = vtkSmartPointer<GeographicTransformationFilter>::New();
    auto poly = generatePoly(CoordinateSystemType::metricLocal);

    filter->SetInputData(poly);
    filter->SetTargetCoordinateSystem(dataSpec_WGS84());
    ASSERT_TRUE(filter->GetExecutive()->Update());
    auto geoDem = filter->GetOutput();
    ASSERT_TRUE(geoDem);

    DataBounds bounds;
    geoDem->GetBounds(bounds.data());
    for (unsigned i = 0; i < 6; ++i)
    {
        ASSERT_NEAR(dataBounds_WGS84()[i], bounds[i], 5.e-8);
    }
}
