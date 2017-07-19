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

#include <vtkDataSet.h>
#include <vtkCellData.h>
#include <vtkImageData.h>
#include <vtkFloatArray.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkTriangle.h>
#include <vtkVertex.h>

#include <core/types.h>
#include <core/data_objects/ImageDataObject.h>
#include <core/data_objects/PointCloudDataObject.h>
#include <core/data_objects/PolyDataObject.h>
#include <core/data_objects/VectorGrid3DDataObject.h>
#include <core/table_model/QVtkTableModel.h>
#include <core/data_objects/DataObject_private.h>
#include <core/utility/DataExtent.h>
#include <core/utility/vtkVector_print.h>


class DataObject_test_DataObject : public DataObject
{
public:
    DataObject_test_DataObject() : DataObject("", nullptr) { }
    DataObject_test_DataObject(const QString & name, vtkDataSet * dataSet)
        : DataObject(name, dataSet)
    {
    }

    std::unique_ptr<DataObject> newInstance(const QString & name, vtkDataSet * dataSet) const override
    {
        return std::make_unique<DataObject_test_DataObject>(name, dataSet);
    }

    bool is3D() const override { return false; }
    IndexType defaultAttributeLocation() const override { return IndexType::invalid; }

    const QString & dataTypeName() const override { static QString empty; return empty; }

    bool isDeferringEvents()
    {
        return dPtr().lockEventDeferrals().isDeferringEvents();
    }

protected:
    std::unique_ptr<QVtkTableModel> createTableModel() override { return nullptr; }
};

using TestDataObject = DataObject_test_DataObject;


class DataObject_test : public ::testing::Test
{
public:
    void SetUp() override
    {
    }
    void TearDown() override
    {
    }

    static vtkSmartPointer<vtkPolyData> createPolyDataPoints(vtkIdType numPoints)
    {
        auto polyData = vtkSmartPointer<vtkPolyData>::New();
        auto points = vtkSmartPointer<vtkPoints>::New();
        points->SetNumberOfPoints(numPoints);
        for (vtkIdType i = 0; i < numPoints; ++i)
        {
            points->SetPoint(i,
                static_cast<double>(i + 1),
                static_cast<double>(numPoints - 1),
                static_cast<double>(numPoints));
        }
        polyData->SetPoints(points);
        return polyData;
    }

    static std::unique_ptr<PointCloudDataObject> createPointCloud(vtkIdType numPoints)
    {
        auto polyData = createPolyDataPoints(numPoints);
        auto vertices = vtkSmartPointer<vtkCellArray>::New();
        auto vertex = vtkSmartPointer<vtkVertex>::New();
        vertex->GetPointIds()->SetId(0, 0);
        vertices->InsertNextCell(vertex);
        polyData->SetVerts(vertices);

        return std::make_unique<PointCloudDataObject>("vertices", *polyData);
    }
};

TEST_F(DataObject_test, newInstance_ImageDataObject)
{
    auto dataSet = vtkSmartPointer<vtkImageData>::New();
    dataSet->SetDimensions(1, 1, 1);
    dataSet->AllocateScalars(VTK_FLOAT, 1);
    auto dataObject1 = std::make_unique<ImageDataObject>("dataObject", *dataSet);
    auto newInstance = dataObject1->newInstance("newInstance", dataSet);
    ASSERT_TRUE(newInstance);
    ASSERT_EQ("newInstance", newInstance->name());
    ASSERT_EQ(dataSet.Get(), newInstance->dataSet());
}

TEST_F(DataObject_test, newInstance_PointCloudDataObject)
{
    auto dataSet = vtkSmartPointer<vtkPolyData>::New();
    auto points = vtkSmartPointer<vtkPoints>::New();
    points->SetNumberOfPoints(1);
    dataSet->SetPoints(points);
    auto verts = vtkSmartPointer<vtkCellArray>::New();
    verts->SetNumberOfCells(1);
    verts->InsertCellPoint(0);
    dataSet->SetVerts(verts);
    auto dataObject1 = std::make_unique<PointCloudDataObject>("dataObject", *dataSet);
    auto newInstance = dataObject1->newInstance("newInstance", dataSet);
    ASSERT_TRUE(newInstance);
    ASSERT_EQ("newInstance", newInstance->name());
    ASSERT_EQ(dataSet.Get(), newInstance->dataSet());
}

TEST_F(DataObject_test, newInstance_PolyDataObject)
{
    auto dataSet = vtkSmartPointer<vtkPolyData>::New();
    auto points = vtkSmartPointer<vtkPoints>::New();
    points->SetNumberOfPoints(3);
    dataSet->SetPoints(points);
    auto polys = vtkSmartPointer<vtkCellArray>::New();
    polys->SetNumberOfCells(1);
    polys->InsertCellPoint(0);
    polys->InsertCellPoint(1);
    polys->InsertCellPoint(2);
    dataSet->SetPolys(polys);
    auto dataObject1 = std::make_unique<PolyDataObject>("dataObject", *dataSet);
    auto newInstance = dataObject1->newInstance("newInstance", dataSet);
    ASSERT_TRUE(newInstance);
    ASSERT_EQ("newInstance", newInstance->name());
    ASSERT_EQ(dataSet.Get(), newInstance->dataSet());
}

TEST_F(DataObject_test, newInstance_VectorGrid3DDataObject)
{
    auto dataSet = vtkSmartPointer<vtkImageData>::New();
    dataSet->SetDimensions(3, 3, 3);
    dataSet->AllocateScalars(VTK_FLOAT, 3);
    auto dataObject1 = std::make_unique<VectorGrid3DDataObject>("dataObject", *dataSet);
    auto newInstance = dataObject1->newInstance("newInstance", dataSet);
    ASSERT_TRUE(newInstance);
    ASSERT_EQ("newInstance", newInstance->name());
    ASSERT_EQ(dataSet.Get(), newInstance->dataSet());
}

TEST_F(DataObject_test, ClearAttributesKeepsNameArray)
{
    const char * const nameArrayName = "Name";
    auto dataSet = vtkSmartPointer<vtkPolyData>::New();
    auto data = std::make_unique<TestDataObject>("noname", dataSet);
    ASSERT_TRUE(dataSet->GetFieldData()->HasArray(nameArrayName));

    data->clearAttributes();
    ASSERT_TRUE(dataSet->GetFieldData()->HasArray(nameArrayName));
}

TEST_F(DataObject_test, ClearAttributesClearsAdditionalAttributes)
{
    auto dataSet = vtkSmartPointer<vtkPolyData>::New();

    auto pointAttr = vtkSmartPointer<vtkFloatArray>::New();
    pointAttr->SetName("pointAttr");
    dataSet->GetPointData()->AddArray(pointAttr);
    auto cellAttr = vtkSmartPointer<vtkFloatArray>::New();
    cellAttr->SetName("cellAttr");
    dataSet->GetCellData()->AddArray(cellAttr);
    auto fieldAttr = vtkSmartPointer<vtkFloatArray>::New();
    fieldAttr->SetName("fieldAttr");
    dataSet->GetFieldData()->AddArray(fieldAttr);

    auto data = std::make_unique<TestDataObject>("noname", dataSet);

    ASSERT_TRUE(dataSet->GetPointData()->HasArray(pointAttr->GetName()));
    ASSERT_TRUE(dataSet->GetCellData()->HasArray(cellAttr->GetName()));
    ASSERT_TRUE(dataSet->GetFieldData()->HasArray(fieldAttr->GetName()));

    data->clearAttributes();

    ASSERT_FALSE(dataSet->GetPointData()->HasArray(pointAttr->GetName()));
    ASSERT_FALSE(dataSet->GetCellData()->HasArray(cellAttr->GetName()));
    ASSERT_FALSE(dataSet->GetFieldData()->HasArray(fieldAttr->GetName()));
}

class IntrinsicAttributesDataObject : public TestDataObject
{
public:
    IntrinsicAttributesDataObject(const QString & name, vtkDataSet * dataSet)
        : TestDataObject(name, dataSet)
    {
    }

protected:
    void addIntrinsicAttributes(
        QStringList & fieldAttributes,
        QStringList & pointAttributes,
        QStringList & cellAttributes) override
    {
        fieldAttributes << "intrFieldAttr";
        pointAttributes << "intrPointAttr";
        cellAttributes << "intrCellAttr";
    }
};

TEST_F(DataObject_test, ClearAttributesKeepsIntrinsicAttributes)
{
    auto dataSet = vtkSmartPointer<vtkPolyData>::New();

    auto pointAttr = vtkSmartPointer<vtkFloatArray>::New();
    pointAttr->SetName("pointAttr");
    dataSet->GetPointData()->AddArray(pointAttr);
    auto cellAttr = vtkSmartPointer<vtkFloatArray>::New();
    cellAttr->SetName("cellAttr");
    dataSet->GetCellData()->AddArray(cellAttr);
    auto fieldAttr = vtkSmartPointer<vtkFloatArray>::New();
    fieldAttr->SetName("fieldAttr");
    dataSet->GetFieldData()->AddArray(fieldAttr);

    auto intrPointAttr = vtkSmartPointer<vtkFloatArray>::New();
    intrPointAttr->SetName("intrPointAttr");
    dataSet->GetPointData()->AddArray(intrPointAttr);
    auto intrCellAttr = vtkSmartPointer<vtkFloatArray>::New();
    intrCellAttr->SetName("intrCellAttr");
    dataSet->GetCellData()->AddArray(intrCellAttr);
    auto intrFieldAttr = vtkSmartPointer<vtkFloatArray>::New();
    intrFieldAttr->SetName("intrFieldAttr");
    dataSet->GetFieldData()->AddArray(intrFieldAttr);

    auto data = std::make_unique<IntrinsicAttributesDataObject>("noname", dataSet);

    ASSERT_TRUE(dataSet->GetPointData()->HasArray(pointAttr->GetName()));
    ASSERT_TRUE(dataSet->GetCellData()->HasArray(cellAttr->GetName()));
    ASSERT_TRUE(dataSet->GetFieldData()->HasArray(fieldAttr->GetName()));
    ASSERT_TRUE(dataSet->GetPointData()->HasArray(intrPointAttr->GetName()));
    ASSERT_TRUE(dataSet->GetCellData()->HasArray(intrCellAttr->GetName()));
    ASSERT_TRUE(dataSet->GetFieldData()->HasArray(intrFieldAttr->GetName()));

    data->clearAttributes();

    ASSERT_FALSE(dataSet->GetPointData()->HasArray(pointAttr->GetName()));
    ASSERT_FALSE(dataSet->GetCellData()->HasArray(cellAttr->GetName()));
    ASSERT_FALSE(dataSet->GetFieldData()->HasArray(fieldAttr->GetName()));
    ASSERT_TRUE(dataSet->GetPointData()->HasArray(intrPointAttr->GetName()));
    ASSERT_TRUE(dataSet->GetCellData()->HasArray(intrCellAttr->GetName()));
    ASSERT_TRUE(dataSet->GetFieldData()->HasArray(intrFieldAttr->GetName()));
}

TEST_F(DataObject_test, CopyStructure_triggers_structureChanged_ImageDataObject)
{
    auto dataSet = vtkSmartPointer<vtkImageData>::New();

    auto referenceDataSet = vtkSmartPointer<vtkImageData>::New();
    referenceDataSet->SetExtent(1, 3, 1, 4, 0, 0);
    referenceDataSet->AllocateScalars(VTK_FLOAT, 1);

    bool structureChangedSignaled = false;

    ImageDataObject image("testImage", *dataSet);
    QObject::connect(&image, &DataObject::structureChanged,
        [&structureChangedSignaled] ()
    {
        structureChangedSignaled = true;
    });

    dataSet->CopyStructure(referenceDataSet);

    ASSERT_TRUE(structureChangedSignaled);
}

TEST_F(DataObject_test, CopyStructure_triggers_structureChanged_PolyDataObject)
{
    auto dataSet = vtkSmartPointer<vtkPolyData>::New();

    auto referenceDataSet = createPolyDataPoints(1);

    bool structureChangedSignaled = false;

    PolyDataObject poly("testPoly", *dataSet);
    QObject::connect(&poly, &DataObject::structureChanged,
        [&structureChangedSignaled] ()
    {
        structureChangedSignaled = true;
    });

    dataSet->CopyStructure(referenceDataSet);

    ASSERT_TRUE(structureChangedSignaled);
}

TEST_F(DataObject_test, CopyStructure_updates_ImageDataObject_extent)
{
    ImageExtent extent({ 1, 3, 1, 4, 0, 0 });

    auto dataSet = vtkSmartPointer<vtkImageData>::New();

    auto referenceDataSet = vtkSmartPointer<vtkImageData>::New();
    referenceDataSet->SetExtent(extent.data());
    referenceDataSet->AllocateScalars(VTK_FLOAT, 1);

    ImageDataObject image("testImage", *dataSet);

    dataSet->CopyStructure(referenceDataSet);

    ASSERT_EQ(extent, image.extent());
}

TEST_F(DataObject_test, CopyStructure_updates_VectorGrid3DDataObject_extent)
{
    ImageExtent extent({ 1, 3, 1, 4, 3, 5 });

    auto dataSet = vtkSmartPointer<vtkImageData>::New();

    auto referenceDataSet = vtkSmartPointer<vtkImageData>::New();
    referenceDataSet->SetExtent(extent.data());
    referenceDataSet->AllocateScalars(VTK_FLOAT, 1);

    VectorGrid3DDataObject grid("testGrid", *dataSet);

    dataSet->CopyStructure(referenceDataSet);

    ASSERT_EQ(extent, grid.extent());
}

TEST_F(DataObject_test, CopyStructure_Modified_updates_PolyDataObject_pointsCells)
{
    const vtkIdType numPoints = 3;
    const vtkIdType numCells = 2;

    auto dataSet = vtkSmartPointer<vtkPolyData>::New();

    auto referenceDataSet = createPolyDataPoints(3);
    auto cells = vtkSmartPointer<vtkCellArray>::New();
    cells->InsertNextCell(3, std::vector<vtkIdType>({ 0, 1, 2 }).data());
    cells->InsertNextCell(3, std::vector<vtkIdType>({ 0, 2, 1 }).data());
    referenceDataSet->SetVerts(cells);

    PolyDataObject poly("testPoly", *dataSet);

    dataSet->CopyStructure(referenceDataSet);
    dataSet->Modified();

    ASSERT_EQ(numPoints, poly.numberOfPoints());
    ASSERT_EQ(numCells, poly.numberOfCells());
}

TEST_F(DataObject_test, DataObject_CopyStructure_updates_PolyDataObject_pointsCells)
{
    const vtkIdType numPoints = 3;
    const vtkIdType numCells = 2;

    auto dataSet = vtkSmartPointer<vtkPolyData>::New();

    auto referenceDataSet = createPolyDataPoints(3);
    auto cells = vtkSmartPointer<vtkCellArray>::New();
    cells->InsertNextCell(3, std::vector<vtkIdType>({ 0, 1, 2 }).data());
    cells->InsertNextCell(3, std::vector<vtkIdType>({ 0, 2, 1 }).data());
    referenceDataSet->SetVerts(cells);

    PolyDataObject poly("testPoly", *dataSet);

    poly.CopyStructure(*referenceDataSet);

    ASSERT_EQ(numPoints, poly.numberOfPoints());
    ASSERT_EQ(numCells, poly.numberOfCells());
}

TEST_F(DataObject_test, GenericPolyDataObject_instantiate_PolyDataObject)
{
    auto polyData = createPolyDataPoints(3);
    auto triangles = vtkSmartPointer<vtkCellArray>::New();
    auto triangle = vtkSmartPointer<vtkTriangle>::New();
    triangle->GetPointIds()->SetId(0, 0);
    triangle->GetPointIds()->SetId(1, 1);
    triangle->GetPointIds()->SetId(2, 2);
    triangles->InsertNextCell(triangle);
    polyData->SetPolys(triangles);

    auto polyDataObject = GenericPolyDataObject::createInstance("triangles", *polyData);
    ASSERT_TRUE(polyDataObject);
    ASSERT_TRUE(dynamic_cast<PolyDataObject *>(polyDataObject.get()));
    ASSERT_EQ(polyData.Get(), polyDataObject->dataSet());
}

TEST_F(DataObject_test, GenericPolyDataObject_instantiate_PointCloudDataObject)
{
    auto polyData = createPolyDataPoints(1);
    auto vertices = vtkSmartPointer<vtkCellArray>::New();
    auto vertex = vtkSmartPointer<vtkVertex>::New();
    vertex->GetPointIds()->SetId(0, 0);
    vertices->InsertNextCell(vertex);
    polyData->SetVerts(vertices);

    auto pointCloudDataObject = GenericPolyDataObject::createInstance("vertices", *polyData);
    ASSERT_TRUE(pointCloudDataObject);
    ASSERT_TRUE(dynamic_cast<PointCloudDataObject *>(pointCloudDataObject.get()));
    ASSERT_EQ(polyData.Get(), pointCloudDataObject->dataSet());
}

namespace
{

struct TransformHelper
{
    vtkVector3d transformedPoint1;
    vtkVector3d transformedPoint2;
    vtkSmartPointer<vtkTransformPolyDataFilter> filter1;
    vtkSmartPointer<vtkTransformPolyDataFilter> filter2;

    static vtkVector3d testPoint()
    {
        return vtkVector3d(8, 16, 64);
    }

    TransformHelper()
        : transformedPoint1{ testPoint() + shift }
        , transformedPoint2{ (testPoint() + shift) * scale }
        , filter1{ vtkSmartPointer<vtkTransformPolyDataFilter>::New() }
        , filter2{ vtkSmartPointer<vtkTransformPolyDataFilter>::New() }
    {
        auto transform1 = vtkSmartPointer<vtkTransform>::New();
        transform1->Translate(shift.GetData());
        filter1->SetTransform(transform1);

        auto transform2 = vtkSmartPointer<vtkTransform>::New();
        transform2->Scale(scale.GetData());
        filter2->SetTransform(transform2);
        filter2->SetInputConnection(filter1->GetOutputPort());
    }

    static const vtkVector3d shift;
    static const vtkVector3d scale;
};

const vtkVector3d TransformHelper::shift = vtkVector3d(8.0, 32.0, 128.0);
const vtkVector3d TransformHelper::scale = vtkVector3d(2.0, 0.5, 0.25);

}

TEST_F(DataObject_test, InjectPostProcessingStep_Simple)
{
    auto dataObject = createPointCloud(1);
    dataObject->polyDataSet().GetPoints()->SetPoint(0, TransformHelper::testPoint().GetData());

    TransformHelper trHelper;
    DataObject::PostProcessingStep ppStep;
    ppStep.pipelineHead = trHelper.filter1;
    ppStep.pipelineTail = trHelper.filter1;

    bool result;
    std::tie(result, std::ignore) = dataObject->injectPostProcessingStep(ppStep);
    ASSERT_TRUE(result);

    auto dataSet = dataObject->processedOutputDataSet();

    ASSERT_TRUE(dataSet);
    ASSERT_EQ(1, dataSet->GetNumberOfPoints());
    ASSERT_EQ(trHelper.transformedPoint1, vtkVector3d(dataSet->GetPoint(0)));
}

TEST_F(DataObject_test, InjectPostProcessingStep_Pipeline)
{
    auto dataObject = createPointCloud(1);
    dataObject->polyDataSet().GetPoints()->SetPoint(0, TransformHelper::testPoint().GetData());

    TransformHelper trHelper;
    DataObject::PostProcessingStep ppStep;
    ppStep.pipelineHead = trHelper.filter1;
    ppStep.pipelineTail = trHelper.filter2;

    bool result;
    std::tie(result, std::ignore) = dataObject->injectPostProcessingStep(ppStep);
    ASSERT_TRUE(result);

    auto dataSet = dataObject->processedOutputDataSet();

    ASSERT_TRUE(dataSet);
    ASSERT_EQ(1, dataSet->GetNumberOfPoints());
    ASSERT_EQ(trHelper.transformedPoint2, vtkVector3d(dataSet->GetPoint(0)));
}

TEST_F(DataObject_test, InjectPostProcessingStep_2Steps)
{
    auto dataObject = createPointCloud(1);
    dataObject->polyDataSet().GetPoints()->SetPoint(0, TransformHelper::testPoint().GetData());

    auto trHelper1 = TransformHelper();
    DataObject::PostProcessingStep ppStep1;
    ppStep1.pipelineHead = trHelper1.filter1;
    ppStep1.pipelineTail = trHelper1.filter2;

    auto trHelper2 = TransformHelper();
    DataObject::PostProcessingStep ppStep2;
    ppStep2.pipelineHead = trHelper2.filter1;
    ppStep2.pipelineTail = trHelper2.filter2;

    bool result;
    std::tie(result, std::ignore) = dataObject->injectPostProcessingStep(ppStep1);
    ASSERT_TRUE(result);
    std::tie(result, std::ignore) = dataObject->injectPostProcessingStep(ppStep2);
    ASSERT_TRUE(result);

    auto dataSet = dataObject->processedOutputDataSet();

    ASSERT_TRUE(dataSet);
    ASSERT_EQ(1, dataSet->GetNumberOfPoints());
    ASSERT_EQ(vtkVector3d((trHelper1.transformedPoint2 + TransformHelper::shift) * TransformHelper::scale),
        vtkVector3d(dataSet->GetPoint(0)));
}

TEST_F(DataObject_test, InjectPostProcessingStep_Erase)
{
    auto dataObject = createPointCloud(1);
    dataObject->polyDataSet().GetPoints()->SetPoint(0, TransformHelper::testPoint().GetData());

    auto trHelper = TransformHelper();

    DataObject::PostProcessingStep ppStep;
    ppStep.pipelineHead = trHelper.filter1;
    ppStep.pipelineTail = trHelper.filter1;

    bool result;
    unsigned int index;
    std::tie(result, index) = dataObject->injectPostProcessingStep(ppStep);
    ASSERT_TRUE(result);
    ASSERT_TRUE(dataObject->processedOutputDataSet());
    ASSERT_TRUE(dataObject->erasePostProcessingStep(index));

    auto dataSet = dataObject->processedOutputDataSet();
    ASSERT_TRUE(dataSet);
    ASSERT_EQ(1, dataSet->GetNumberOfPoints());
    ASSERT_EQ(trHelper.testPoint(), vtkVector3d(dataSet->GetPoint(0)));
}

TEST_F(DataObject_test, InjectPostProcessingStep_2Steps_Erase1)
{
    auto dataObject = createPointCloud(1);
    dataObject->polyDataSet().GetPoints()->SetPoint(0, TransformHelper::testPoint().GetData());

    auto trHelper1 = TransformHelper();
    DataObject::PostProcessingStep ppStep1;
    ppStep1.pipelineHead = trHelper1.filter1;
    ppStep1.pipelineTail = trHelper1.filter2;

    auto trHelper2 = TransformHelper();
    DataObject::PostProcessingStep ppStep2;
    ppStep2.pipelineHead = trHelper2.filter1;
    ppStep2.pipelineTail = trHelper2.filter2;

    bool result;
    unsigned int index1, index2;
    std::tie(result, index1) = dataObject->injectPostProcessingStep(ppStep1);
    ASSERT_TRUE(result);
    std::tie(result, index2) = dataObject->injectPostProcessingStep(ppStep2);
    ASSERT_TRUE(result);
    ASSERT_NE(index1, index2);

    ASSERT_TRUE(dataObject->processedOutputDataSet());

    ASSERT_TRUE(dataObject->erasePostProcessingStep(index1));

    auto dataSet = dataObject->processedOutputDataSet();
    ASSERT_TRUE(dataSet);
    ASSERT_EQ(1, dataSet->GetNumberOfPoints());
    ASSERT_EQ(trHelper2.transformedPoint2, vtkVector3d(dataSet->GetPoint(0)));
}

TEST_F(DataObject_test, InjectPostProcessingStep_2Steps_Erase1_Inject)
{
    auto dataObject = createPointCloud(1);
    dataObject->polyDataSet().GetPoints()->SetPoint(0, TransformHelper::testPoint().GetData());

    auto trHelper1 = TransformHelper();
    DataObject::PostProcessingStep ppStep1;
    ppStep1.pipelineHead = trHelper1.filter1;
    ppStep1.pipelineTail = trHelper1.filter2;

    auto trHelper2 = TransformHelper();
    DataObject::PostProcessingStep ppStep2;
    ppStep2.pipelineHead = trHelper2.filter1;
    ppStep2.pipelineTail = trHelper2.filter2;

    bool result;
    unsigned int index1, index2;
    std::tie(result, index1) = dataObject->injectPostProcessingStep(ppStep1);
    ASSERT_TRUE(result);
    std::tie(result, index2) = dataObject->injectPostProcessingStep(ppStep2);
    ASSERT_TRUE(result);
    ASSERT_NE(index1, index2);

    ASSERT_TRUE(dataObject->processedOutputDataSet());

    ASSERT_TRUE(dataObject->erasePostProcessingStep(index1));

    std::tie(result, index1) = dataObject->injectPostProcessingStep(ppStep1);
    ASSERT_TRUE(result);
    ASSERT_NE(index1, index2);

    auto dataSet = dataObject->processedOutputDataSet();
    ASSERT_TRUE(dataSet);
    ASSERT_EQ(1, dataSet->GetNumberOfPoints());
    ASSERT_EQ(vtkVector3d((trHelper1.transformedPoint2 + TransformHelper::shift) * TransformHelper::scale),
        vtkVector3d(dataSet->GetPoint(0)));
}

TEST(ScopedEventDeferral_test, ExecutesDeferred)
{
    TestDataObject data("noname", vtkSmartPointer<vtkPolyData>::New());

    bool isSignaled = false;

    QObject::connect(&data, &DataObject::attributeArraysChanged, [&isSignaled] () -> void
    {
        isSignaled = true;
    });

    {
        ScopedEventDeferral d0(data);

        data.dataSet()->GetPointData()->Modified();

        ASSERT_FALSE(isSignaled);
    }

    ASSERT_TRUE(isSignaled);
}

TEST(ScopedEventDeferral_test, move_lock)
{
    TestDataObject data;

    {
        ASSERT_FALSE(data.isDeferringEvents());

        ScopedEventDeferral d0(data);

        ASSERT_TRUE(data.isDeferringEvents());

        auto moveDeferral = [&data]() -> ScopedEventDeferral
        {
            ScopedEventDeferral d(data);

#if defined (__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpessimizing-move"
#endif
            return std::move(d);
#if defined (__clang__)
#pragma clang diagnostic pop
#endif
        };

        auto d2 = moveDeferral();

        ASSERT_TRUE(data.isDeferringEvents());
    }

    ASSERT_FALSE(data.isDeferringEvents());
}
