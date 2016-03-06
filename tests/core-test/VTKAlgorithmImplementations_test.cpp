#include <gtest/gtest.h>

#include <vtkFloatArray.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkRearrangeFields.h>
#include <vtkSmartPointer.h>


TEST(vtkRearrangeFields_test, assign_to_same_attribute_removes_attribute_flag)
{
    auto dataSet = vtkSmartPointer<vtkPolyData>::New();
    auto normals = vtkSmartPointer<vtkFloatArray>::New();
    normals->SetName("someNormals");
    normals->SetNumberOfComponents(3);
    dataSet->GetPointData()->SetNormals(normals);

    auto rearrangeFields = vtkSmartPointer<vtkRearrangeFields>::New();
    rearrangeFields->AddOperation(vtkRearrangeFields::MOVE,
        vtkDataSetAttributes::NORMALS, 
        vtkRearrangeFields::POINT_DATA,
        vtkRearrangeFields::POINT_DATA);
    rearrangeFields->SetInputData(dataSet);
    rearrangeFields->Update();

    auto outputDataSet = rearrangeFields->GetOutput();

    ASSERT_EQ(nullptr, outputDataSet->GetPointData()->GetNormals());
}
