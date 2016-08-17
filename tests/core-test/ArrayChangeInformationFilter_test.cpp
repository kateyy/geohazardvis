#include <gtest/gtest.h>

#include <vtkCellData.h>
#include <vtkFloatArray.h>
#include <vtkInformation.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>
#include <vtkVersionMacros.h>

#include <core/filters/ArrayChangeInformationFilter.h>
#include <core/utility/macros.h>


class ArrayChangeInformationFilter_test : public testing::Test
{
public:
    void SetUp() override
    {
        inAttr->SetName("in");
        filter->SetInputData(inDs);
    }

    vtkDataArray * getScalars()
    {
        return filter->GetOutput()->GetPointData()->GetScalars();
    }

    vtkSmartPointer<vtkPolyData> inDs = vtkSmartPointer<vtkPolyData>::New();
    vtkSmartPointer<vtkFloatArray> inAttr = vtkSmartPointer<vtkFloatArray>::New();

    vtkSmartPointer<ArrayChangeInformationFilter> filter = vtkSmartPointer<ArrayChangeInformationFilter>::New();
};


#if VTK_CHECK_VERSION(7, 1, 0)
#define CHECKED_VTK_VERSION(TEST_NAME) TEST_NAME
#else
#define CHECKED_VTK_VERSION(TEST_NAME) DISABLED_##TEST_NAME
#endif

TEST_F(ArrayChangeInformationFilter_test, DoRename)
{
    inDs->GetPointData()->SetScalars(inAttr);

    filter->SetArrayName("out");
    filter->Update();

    auto outAttr = getScalars();

    ASSERT_TRUE(outAttr);
    ASSERT_STREQ("out", outAttr->GetName());
}

TEST_F(ArrayChangeInformationFilter_test, CHECKED_VTK_VERSION(DoSetUnit))
{
CHECKED_VTK_VERSION(
    inDs->GetPointData()->SetScalars(inAttr);

    filter->EnableSetUnitOn();
    filter->SetArrayUnit("someUnit");
    filter->Update();

    auto outAttr = getScalars();

    ASSERT_STREQ("someUnit", outAttr->GetInformation()->Get(vtkDataArray::UNITS_LABEL()));
    )
}

TEST_F(ArrayChangeInformationFilter_test, CreateArrayCopy)
{
    inDs->GetPointData()->SetScalars(inAttr);

    filter->Update();

    auto outAttr = getScalars();

    ASSERT_NE(inAttr.Get(), outAttr);
}

TEST_F(ArrayChangeInformationFilter_test, DontChangeInArray)
{
    inAttr->SetName("someName");
    filter->SetArrayName("otherName");

    CHECKED_VTK_VERSION(
        inAttr->GetInformation()->Set(vtkDataArray::UNITS_LABEL(), "someUnit");
        filter->SetArrayUnit("otherUnit");
        filter->EnableSetUnitOn();
    )

    inDs->GetPointData()->SetScalars(inAttr);

    filter->Update();

    ASSERT_STREQ("someName", inAttr->GetName());

    CHECKED_VTK_VERSION(
        ASSERT_STREQ("someUnit", inAttr->GetInformation()->Get(vtkDataArray::UNITS_LABEL()));
    )
}

TEST_F(ArrayChangeInformationFilter_test, AttributeLocationAndType)
{
    inAttr->SetNumberOfComponents(3);
    inAttr->SetName("inNormals");
    inDs->GetCellData()->SetNormals(inAttr);

    filter->SetAttributeLocation(ArrayChangeInformationFilter::CELL_DATA);
    filter->SetAttributeType(vtkDataSetAttributes::NORMALS);
    filter->SetArrayName("outNormals");
    filter->Update();

    auto outNormals = filter->GetOutput()->GetCellData()->GetNormals();

    ASSERT_TRUE(outNormals);
    ASSERT_STREQ("outNormals", outNormals->GetName());
}
