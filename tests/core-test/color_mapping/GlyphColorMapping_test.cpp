#include <gtest/gtest.h>

#include <cassert>
#include <algorithm>
#include <tuple>

#include <QDebug>

#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkFloatArray.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>

#include <core/types.h>
#include <core/data_objects/PointCloudDataObject.h>
#include <core/data_objects/PolyDataObject.h>
#include <core/rendered_data/RenderedData3D.h>
#include <core/color_mapping/ColorMapping.h>
#include <core/color_mapping/GlyphMagnitudeColorMapping.h>
#include <core/glyph_mapping/GlyphMapping.h>
#include <core/glyph_mapping/GlyphMappingData.h>


class GlyphColorMapping_test : public ::testing::Test
{
public:
    std::tuple<vtkSmartPointer<vtkPolyData>, vtkSmartPointer<vtkCellArray>, vtkSmartPointer<vtkFloatArray>> genvtkPolyData()
    {
        auto points = vtkSmartPointer<vtkPoints>::New();
        points->InsertNextPoint(0, 0, 0);
        points->InsertNextPoint(0, 1, 0);
        points->InsertNextPoint(1, 1, 0);

        auto poly = vtkSmartPointer<vtkPolyData>::New();
        poly->SetPoints(points);

        auto indices = vtkSmartPointer<vtkCellArray>::New();
        indices->InsertNextCell(3);
        indices->InsertCellPoint(0);
        indices->InsertCellPoint(1);
        indices->InsertCellPoint(2);

        auto vectors = vtkSmartPointer<vtkFloatArray>::New();
        vectors->SetName(vectorName());
        vectors->SetNumberOfComponents(3);
        vectors->SetNumberOfTuples(1);
        vectors->SetTypedComponent(0, 0, 0);
        vectors->SetTypedComponent(0, 1, 0);
        vectors->SetTypedComponent(0, 2, 1);

        return { poly, indices, vectors };
    }

    std::unique_ptr<PolyDataObject> genPolyData()
    {
        auto tuple = genvtkPolyData();
        auto && poly = std::get<0>(tuple);

        poly->SetPolys(std::get<1>(tuple));
        poly->GetCellData()->SetVectors(std::get<2>(tuple));

        return std::make_unique<PolyDataObject>("Poly3", *poly);
    }

    std::unique_ptr<PointCloudDataObject> genPointCloud()
    {
        auto tuple = genvtkPolyData();
        auto && poly = std::get<0>(tuple);

        poly->SetVerts(std::get<1>(tuple));
        poly->GetPointData()->SetVectors(std::get<2>(tuple));

        return std::make_unique<PointCloudDataObject>("PointCloud", *poly);
    }

    static const char * vectorName()
    {
        static const char * name = "Vectors";
        return name;
    }

    static const QString & glyphMagnitudeColorMappingName()
    {
        static const QString name = QString("Glyph: %1 Magnitude").arg(vectorName());
        return name;
    }
};

TEST_F(GlyphColorMapping_test, GlyphMagnitudeColorMappingCreated)
{
    auto poly = genPolyData();
    auto rendered = poly->createRendered();
    auto rendered3D = dynamic_cast<RenderedData3D *>(rendered.get());
    assert(rendered3D);

    auto glyphMappings = rendered3D->glyphMapping().vectors();
    auto vecGlyphMappingIt = std::find_if(glyphMappings.begin(), glyphMappings.end(),
        [] (GlyphMappingData * glyphMappingData)
    {
        assert(glyphMappingData);
        return glyphMappingData->name() == GlyphColorMapping_test::vectorName();
    });
    ASSERT_NE(glyphMappings.end(), vecGlyphMappingIt);

    auto vecGlyphMapping = *vecGlyphMappingIt;
    vecGlyphMapping->setVisible(true);

    auto & colorMapping = rendered3D->colorMapping();

    ASSERT_TRUE(colorMapping.scalarsNames().contains(glyphMagnitudeColorMappingName()));
    colorMapping.setCurrentScalarsByName(glyphMagnitudeColorMappingName(), true);

    auto glyphColorMapping = dynamic_cast<GlyphMagnitudeColorMapping *>(&colorMapping.currentScalars());
    ASSERT_TRUE(glyphColorMapping);
}

TEST_F(GlyphColorMapping_test, ReportScalarAssociationCells)
{
    auto poly = genPolyData();
    auto rendered = poly->createRendered();
    auto rendered3D = dynamic_cast<RenderedData3D *>(rendered.get());
    assert(rendered3D);

    auto glyphMappings = rendered3D->glyphMapping().vectors();
    auto vecGlyphMappingIt = std::find_if(glyphMappings.begin(), glyphMappings.end(),
        [] (GlyphMappingData * glyphMappingData)
    {
        assert(glyphMappingData);
        return glyphMappingData->name() == GlyphColorMapping_test::vectorName();
    });
    ASSERT_NE(glyphMappings.end(), vecGlyphMappingIt);

    auto vecGlyphMapping = *vecGlyphMappingIt;
    vecGlyphMapping->setVisible(true);

    auto & colorMapping = rendered3D->colorMapping();

    ASSERT_TRUE(colorMapping.scalarsNames().contains(glyphMagnitudeColorMappingName()));
    colorMapping.setCurrentScalarsByName(glyphMagnitudeColorMappingName(), true);

    auto glyphColorMapping = dynamic_cast<GlyphMagnitudeColorMapping *>(&colorMapping.currentScalars());
    ASSERT_TRUE(glyphColorMapping);

    ASSERT_EQ(IndexType::cells, glyphColorMapping->scalarsAssociation(*rendered));
}

// Not implemented yet
// TEST_F(GlyphColorMapping_test, ReportScalarAssociationPoints)
// {
//     auto poly = genPointCloud();
//     auto rendered = poly->createRendered();
//     auto rendered3D = dynamic_cast<RenderedData3D *>(rendered.get());
//     assert(rendered3D);
//
//     auto glyphMappings = rendered3D->glyphMapping().vectors();
//     auto vecGlyphMappingIt = std::find_if(glyphMappings.begin(), glyphMappings.end(),
//         [] (GlyphMappingData * glyphMappingData)
//     {
//         assert(glyphMappingData);
//         return glyphMappingData->name() == GlyphColorMapping_test::vectorName();
//     });
//     ASSERT_NE(glyphMappings.end(), vecGlyphMappingIt);
//
//     auto vecGlyphMapping = *vecGlyphMappingIt;
//     vecGlyphMapping->setVisible(true);
//
//     auto & colorMapping = rendered3D->colorMapping();
//
//     ASSERT_TRUE(colorMapping.scalarsNames().contains(glyphMagnitudeColorMappingName()));
//     colorMapping.setCurrentScalarsByName(glyphMagnitudeColorMappingName(), true);
//
//     auto glyphColorMapping = dynamic_cast<GlyphMagnitudeColorMapping *>(&colorMapping.currentScalars());
//     ASSERT_TRUE(glyphColorMapping);
//
//     ASSERT_EQ(IndexType::cells, glyphColorMapping->scalarsAssociation(*rendered));
// }
