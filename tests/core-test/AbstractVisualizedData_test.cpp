#include <gtest/gtest.h>

#include <tuple>

#include <vtkCellArray.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkVector.h>

#include <reflectionzeug/PropertyGroup.h>

#include <core/AbstractVisualizedData.h>
#include <core/types.h>
#include <core/data_objects/PointCloudDataObject.h>
#include <core/utility/DataExtent.h>
#include <core/utility/vtkVector_print.h>


class AbstractVisualizedData_test : public ::testing::Test
{
public:
    
    static vtkVector3d testPoint()
    {
        return vtkVector3d(8, 16, 64);
    }

    PointCloudDataObject & testDataObject()
    {
        if (m_poly)
        {
            return *m_poly;
        }

        auto points = vtkSmartPointer<vtkPoints>::New();
        points->SetDataTypeToDouble();
        points->SetNumberOfPoints(1);
        points->SetPoint(0, testPoint().GetData());

        auto verts = vtkSmartPointer<vtkCellArray>::New();
        verts->InsertNextCell(1, std::vector<vtkIdType>({ 0 }).data());

        auto poly = vtkSmartPointer<vtkPolyData>::New();
        poly->SetPoints(points);
        poly->SetVerts(verts);

        m_poly = std::make_unique<PointCloudDataObject>("ADataObject", *poly);
        return *m_poly;
    }

    class TestVisualization : public AbstractVisualizedData
    {
    public:
        TestVisualization(DataObject & dataObject)
            : AbstractVisualizedData(ContentType::Rendered3D, dataObject)
        {
        }
        std::unique_ptr<reflectionzeug::PropertyGroup> createConfigGroup() override { return nullptr; }
        DataBounds updateVisibleBounds() override { return{}; }
    };

    struct TransformHelper
    {
        vtkVector3d transformedPoint1;
        vtkVector3d transformedPoint2;
        vtkSmartPointer<vtkTransformPolyDataFilter> filter1;
        vtkSmartPointer<vtkTransformPolyDataFilter> filter2;

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

private:
    std::unique_ptr<PointCloudDataObject> m_poly;
};

const vtkVector3d AbstractVisualizedData_test::TransformHelper::shift = vtkVector3d(8.0, 32.0, 128.0);
const vtkVector3d AbstractVisualizedData_test::TransformHelper::scale = vtkVector3d(2.0, 0.5, 0.25);


TEST_F(AbstractVisualizedData_test, InjectPostProcessingStepBaseline)
{
    auto vis = std::make_unique<TestVisualization>(testDataObject());

    auto dataSet = vis->processedOutputDataSet();
    ASSERT_TRUE(dataSet);
    ASSERT_EQ(1, dataSet->GetNumberOfPoints());
    ASSERT_EQ(testPoint(), vtkVector3d(dataSet->GetPoint(0)));
}

TEST_F(AbstractVisualizedData_test, InjectPostProcessingStep_Simple)
{
    auto vis = std::make_unique<TestVisualization>(testDataObject());

    auto trHelper = TransformHelper();

    AbstractVisualizedData::PostProcessingStep ppStep;
    ppStep.visualizationPort = 0;
    ppStep.pipelineHead = trHelper.filter1;
    ppStep.pipelineTail = trHelper.filter1;

    bool result;
    std::tie(result, std::ignore) = vis->injectPostProcessingStep(ppStep);
    ASSERT_TRUE(result);

    auto dataSet = vis->processedOutputDataSet();

    ASSERT_TRUE(dataSet);
    ASSERT_EQ(1, dataSet->GetNumberOfPoints());
    ASSERT_EQ(trHelper.transformedPoint1, vtkVector3d(dataSet->GetPoint(0)));
}

TEST_F(AbstractVisualizedData_test, InjectPostProcessingStep_Pipeline)
{
    auto vis = std::make_unique<TestVisualization>(testDataObject());

    auto trHelper = TransformHelper();

    AbstractVisualizedData::PostProcessingStep ppStep;
    ppStep.visualizationPort = 0;
    ppStep.pipelineHead = trHelper.filter1;
    ppStep.pipelineTail = trHelper.filter2;

    bool result;
    std::tie(result, std::ignore) = vis->injectPostProcessingStep(ppStep);
    ASSERT_TRUE(result);

    auto dataSet = vis->processedOutputDataSet();

    ASSERT_TRUE(dataSet);
    ASSERT_EQ(1, dataSet->GetNumberOfPoints());
    ASSERT_EQ(trHelper.transformedPoint2, vtkVector3d(dataSet->GetPoint(0)));
}

TEST_F(AbstractVisualizedData_test, InjectPostProcessingStep_2Steps)
{
    auto vis = std::make_unique<TestVisualization>(testDataObject());

    auto trHelper1 = TransformHelper();
    AbstractVisualizedData::PostProcessingStep ppStep1;
    ppStep1.visualizationPort = 0;
    ppStep1.pipelineHead = trHelper1.filter1;
    ppStep1.pipelineTail = trHelper1.filter2;

    auto trHelper2 = TransformHelper();
    AbstractVisualizedData::PostProcessingStep ppStep2;
    ppStep2.visualizationPort = 0;
    ppStep2.pipelineHead = trHelper2.filter1;
    ppStep2.pipelineTail = trHelper2.filter2;

    bool result;
    std::tie(result, std::ignore) = vis->injectPostProcessingStep(ppStep1);
    ASSERT_TRUE(result);
    std::tie(result, std::ignore) = vis->injectPostProcessingStep(ppStep2);
    ASSERT_TRUE(result);

    auto dataSet = vis->processedOutputDataSet();

    ASSERT_TRUE(dataSet);
    ASSERT_EQ(1, dataSet->GetNumberOfPoints());
    ASSERT_EQ(vtkVector3d((trHelper1.transformedPoint2 + TransformHelper::shift) * TransformHelper::scale),
        vtkVector3d(dataSet->GetPoint(0)));
}

TEST_F(AbstractVisualizedData_test, InjectPostProcessingStep_Erase)
{
    auto vis = std::make_unique<TestVisualization>(testDataObject());

    auto trHelper = TransformHelper();

    AbstractVisualizedData::PostProcessingStep ppStep;
    ppStep.visualizationPort = 0;
    ppStep.pipelineHead = trHelper.filter1;
    ppStep.pipelineTail = trHelper.filter1;

    bool result;
    unsigned int index;
    std::tie(result, index) = vis->injectPostProcessingStep(ppStep);
    ASSERT_TRUE(result);
    ASSERT_TRUE(vis->processedOutputDataSet());
    ASSERT_TRUE(vis->erasePostProcessingStep(index));

    auto dataSet = vis->processedOutputDataSet();
    ASSERT_TRUE(dataSet);
    ASSERT_EQ(1, dataSet->GetNumberOfPoints());
    ASSERT_EQ(testPoint(), vtkVector3d(dataSet->GetPoint(0)));
}

TEST_F(AbstractVisualizedData_test, InjectPostProcessingStep_2Steps_Erase1)
{
    auto vis = std::make_unique<TestVisualization>(testDataObject());

    auto trHelper1 = TransformHelper();
    AbstractVisualizedData::PostProcessingStep ppStep1;
    ppStep1.visualizationPort = 0;
    ppStep1.pipelineHead = trHelper1.filter1;
    ppStep1.pipelineTail = trHelper1.filter2;

    auto trHelper2 = TransformHelper();
    AbstractVisualizedData::PostProcessingStep ppStep2;
    ppStep2.visualizationPort = 0;
    ppStep2.pipelineHead = trHelper2.filter1;
    ppStep2.pipelineTail = trHelper2.filter2;

    bool result;
    unsigned int index1, index2;
    std::tie(result, index1) = vis->injectPostProcessingStep(ppStep1);
    ASSERT_TRUE(result);
    std::tie(result, index2) = vis->injectPostProcessingStep(ppStep2);
    ASSERT_TRUE(result);
    ASSERT_NE(index1, index2);

    vis->processedOutputDataSet();

    ASSERT_TRUE(vis->erasePostProcessingStep(index1));

    auto dataSet = vis->processedOutputDataSet();
    ASSERT_TRUE(dataSet);
    ASSERT_EQ(1, dataSet->GetNumberOfPoints());
    ASSERT_EQ(trHelper2.transformedPoint2, vtkVector3d(dataSet->GetPoint(0)));
}

TEST_F(AbstractVisualizedData_test, InjectPostProcessingStep_2Steps_Erase1_Inject)
{
    auto vis = std::make_unique<TestVisualization>(testDataObject());

    auto trHelper1 = TransformHelper();
    AbstractVisualizedData::PostProcessingStep ppStep1;
    ppStep1.visualizationPort = 0;
    ppStep1.pipelineHead = trHelper1.filter1;
    ppStep1.pipelineTail = trHelper1.filter2;

    auto trHelper2 = TransformHelper();
    AbstractVisualizedData::PostProcessingStep ppStep2;
    ppStep2.visualizationPort = 0;
    ppStep2.pipelineHead = trHelper2.filter1;
    ppStep2.pipelineTail = trHelper2.filter2;

    bool result;
    unsigned int index1, index2;
    std::tie(result, index1) = vis->injectPostProcessingStep(ppStep1);
    ASSERT_TRUE(result);
    std::tie(result, index2) = vis->injectPostProcessingStep(ppStep2);
    ASSERT_TRUE(result);
    ASSERT_NE(index1, index2);

    vis->processedOutputDataSet();

    ASSERT_TRUE(vis->erasePostProcessingStep(index1));

    std::tie(result, index1) = vis->injectPostProcessingStep(ppStep1);
    ASSERT_TRUE(result);
    ASSERT_NE(index1, index2);

    auto dataSet = vis->processedOutputDataSet();
    ASSERT_TRUE(dataSet);
    ASSERT_EQ(1, dataSet->GetNumberOfPoints());
    ASSERT_EQ(vtkVector3d((trHelper1.transformedPoint2 + TransformHelper::shift) * TransformHelper::scale),
        vtkVector3d(dataSet->GetPoint(0)));
}
