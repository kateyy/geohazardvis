#include <gtest/gtest.h>

#include <vtkAssignAttribute.h>
#include <vtkCellData.h>
#include <vtkFloatArray.h>
#include <vtkImageData.h>
#include <vtkImageNormalize.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>

#include <core/filters/ArrayChangeInformationFilter.h>
#include <core/utility/macros.h>

#include "PipelineInformationHelper.h"


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

TEST_F(ArrayChangeInformationFilter_test, DoRename)
{
    inDs->GetPointData()->SetScalars(inAttr);

    filter->SetArrayName("out");
    filter->Update();

    auto outAttr = getScalars();

    ASSERT_TRUE(outAttr);
    ASSERT_STREQ("out", outAttr->GetName());
}

TEST_F(ArrayChangeInformationFilter_test, DoSetUnit)
{
    inDs->GetPointData()->SetScalars(inAttr);

    filter->EnableSetUnitOn();
    filter->SetArrayUnit("someUnit");
    filter->Update();

    auto outAttr = getScalars();

    ASSERT_STREQ("someUnit", outAttr->GetInformation()->Get(vtkDataArray::UNITS_LABEL()));
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

    inAttr->GetInformation()->Set(vtkDataArray::UNITS_LABEL(), "someUnit");
    filter->SetArrayUnit("otherUnit");
    filter->EnableSetUnitOn();

    inDs->GetPointData()->SetScalars(inAttr);

    filter->Update();

    ASSERT_STREQ("someName", inAttr->GetName());

    ASSERT_STREQ("someUnit", inAttr->GetInformation()->Get(vtkDataArray::UNITS_LABEL()));
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

TEST_F(ArrayChangeInformationFilter_test, KeepActiveVectors)
{
    inAttr->SetNumberOfComponents(3);
    inAttr->SetName("inVectors");
    inDs->GetPointData()->SetVectors(inAttr);

    filter->SetAttributeLocation(ArrayChangeInformationFilter::POINT_DATA);
    filter->SetAttributeType(vtkDataSetAttributes::VECTORS);
    filter->SetArrayName("outVectors");
    filter->Update();

    auto outVectors = filter->GetOutput()->GetPointData()->GetVectors();

    ASSERT_TRUE(outVectors);
    ASSERT_STREQ("outVectors", outVectors->GetName());
}

TEST_F(ArrayChangeInformationFilter_test, PassScalarInformation)
{
    inAttr->SetNumberOfComponents(5);
    inAttr->SetNumberOfTuples(17);
    auto inImage = vtkSmartPointer<vtkImageData>::New();
    inImage->GetPointData()->SetScalars(inAttr);
    inImage->SetExtent(0, 0, 2, 18, 3, 3);

    auto infoSource = vtkSmartPointer<InformationSource>::New();
    infoSource->SetOutput(inImage);

    vtkDataObject::SetActiveAttributeInfo(infoSource->GetOutInfo(),
        vtkDataObject::FIELD_ASSOCIATION_POINTS,
        vtkDataSetAttributes::SCALARS,
        inAttr->GetName(),
        inAttr->GetDataType(),
        inAttr->GetNumberOfComponents(),
        static_cast<int>(inAttr->GetNumberOfTuples()));

    filter->SetAttributeLocation(ArrayChangeInformationFilter::POINT_DATA);
    filter->SetAttributeType(vtkDataSetAttributes::SCALARS);
    filter->SetInputConnection(infoSource->GetOutputPort());
    filter->EnableRenameOff();
    filter->EnableSetUnitOff();

    auto getInfo = vtkSmartPointer<InformationSink>::New();
    getInfo->SetInputConnection(filter->GetOutputPort());

    getInfo->UpdateInformation();

    auto outInfo = vtkDataObject::GetActiveFieldInformation(getInfo->GetInInfo(),
        vtkDataObject::FIELD_ASSOCIATION_POINTS,
        vtkDataSetAttributes::SCALARS);

    ASSERT_TRUE(outInfo);

    ASSERT_TRUE(outInfo->Has(vtkDataObject::FIELD_NUMBER_OF_COMPONENTS()));
    ASSERT_EQ(inAttr->GetNumberOfComponents(), outInfo->Get(vtkDataObject::FIELD_NUMBER_OF_COMPONENTS()));

    ASSERT_TRUE(outInfo->Has(vtkDataObject::FIELD_NUMBER_OF_TUPLES()));
    ASSERT_EQ(inAttr->GetNumberOfTuples(), outInfo->Get(vtkDataObject::FIELD_NUMBER_OF_TUPLES()));

    ASSERT_TRUE(outInfo->Has(vtkDataObject::FIELD_NAME()));
    ASSERT_STREQ(inAttr->GetName(), outInfo->Get(vtkDataObject::FIELD_NAME()));

    ASSERT_TRUE(outInfo->Has(vtkDataObject::FIELD_ARRAY_TYPE()));
    ASSERT_EQ(inAttr->GetDataType(), outInfo->Get(vtkDataObject::FIELD_ARRAY_TYPE()));
}

TEST_F(ArrayChangeInformationFilter_test, PassVectorInformation)
{
    inAttr->SetNumberOfComponents(3);
    inAttr->SetNumberOfTuples(17);
    auto inImage = vtkSmartPointer<vtkImageData>::New();
    inImage->GetPointData()->SetVectors(inAttr);
    inImage->SetExtent(0, 0, 2, 18, 3, 3);

    auto infoSource = vtkSmartPointer<InformationSource>::New();
    infoSource->SetOutput(inImage);

    vtkDataObject::SetActiveAttributeInfo(infoSource->GetOutInfo(),
        vtkDataObject::FIELD_ASSOCIATION_POINTS,
        vtkDataSetAttributes::VECTORS,
        inAttr->GetName(),
        inAttr->GetDataType(),
        inAttr->GetNumberOfComponents(),
        static_cast<int>(inAttr->GetNumberOfTuples()));

    filter->SetAttributeLocation(ArrayChangeInformationFilter::POINT_DATA);
    filter->SetAttributeType(vtkDataSetAttributes::VECTORS);
    filter->SetInputConnection(infoSource->GetOutputPort());
    filter->EnableRenameOff();
    filter->EnableSetUnitOff();

    auto getInfo = vtkSmartPointer<InformationSink>::New();
    getInfo->SetInputConnection(filter->GetOutputPort());

    getInfo->UpdateInformation();

    auto outInfo = vtkDataObject::GetActiveFieldInformation(getInfo->GetInInfo(),
        vtkDataObject::FIELD_ASSOCIATION_POINTS,
        vtkDataSetAttributes::VECTORS);

    ASSERT_TRUE(outInfo);

    ASSERT_TRUE(outInfo->Has(vtkDataObject::FIELD_NUMBER_OF_COMPONENTS()));
    ASSERT_EQ(inAttr->GetNumberOfComponents(), outInfo->Get(vtkDataObject::FIELD_NUMBER_OF_COMPONENTS()));

    ASSERT_TRUE(outInfo->Has(vtkDataObject::FIELD_NUMBER_OF_TUPLES()));
    ASSERT_EQ(inAttr->GetNumberOfTuples(), outInfo->Get(vtkDataObject::FIELD_NUMBER_OF_TUPLES()));

    ASSERT_TRUE(outInfo->Has(vtkDataObject::FIELD_NAME()));
    ASSERT_STREQ(inAttr->GetName(), outInfo->Get(vtkDataObject::FIELD_NAME()));

    ASSERT_TRUE(outInfo->Has(vtkDataObject::FIELD_ARRAY_TYPE()));
    ASSERT_EQ(inAttr->GetDataType(), outInfo->Get(vtkDataObject::FIELD_ARRAY_TYPE()));
}

TEST_F(ArrayChangeInformationFilter_test, CheckWhichInformation_vtkAssignAttribute_passes)
{
    inAttr->SetNumberOfComponents(3);
    inAttr->SetNumberOfTuples(17);
    auto inImage = vtkSmartPointer<vtkImageData>::New();
    inImage->GetPointData()->SetScalars(inAttr);
    inImage->SetExtent(0, 0, 2, 18, 3, 3);

    auto infoSource = vtkSmartPointer<InformationSource>::New();
    infoSource->SetOutput(inImage);

    vtkDataObject::SetActiveAttributeInfo(infoSource->GetOutInfo(),
        vtkDataObject::FIELD_ASSOCIATION_POINTS,
        vtkDataSetAttributes::SCALARS,
        inAttr->GetName(),
        inAttr->GetDataType(),
        inAttr->GetNumberOfComponents(),
        static_cast<int>(inAttr->GetNumberOfTuples()));

    filter->SetAttributeLocation(ArrayChangeInformationFilter::POINT_DATA);
    filter->SetAttributeType(vtkDataSetAttributes::SCALARS);
    filter->SetInputConnection(infoSource->GetOutputPort());
    filter->EnableRenameOff();
    filter->EnableSetUnitOff();

    auto assignToVectors = vtkSmartPointer<vtkAssignAttribute>::New();
    assignToVectors->SetInputConnection(filter->GetOutputPort());
    assignToVectors->Assign(vtkDataSetAttributes::SCALARS, vtkDataSetAttributes::VECTORS,
        vtkAssignAttribute::POINT_DATA);

    auto getInfo = vtkSmartPointer<InformationSink>::New();
    getInfo->SetInputConnection(assignToVectors->GetOutputPort());

    getInfo->UpdateInformation();

    auto outScalarInfo = vtkDataObject::GetActiveFieldInformation(getInfo->GetInInfo(),
        vtkDataObject::FIELD_ASSOCIATION_POINTS,
        vtkDataSetAttributes::SCALARS);

    ASSERT_TRUE(outScalarInfo);

    ASSERT_TRUE(outScalarInfo->Has(vtkDataObject::FIELD_NUMBER_OF_COMPONENTS()));
    ASSERT_EQ(inAttr->GetNumberOfComponents(), outScalarInfo->Get(vtkDataObject::FIELD_NUMBER_OF_COMPONENTS()));

    ASSERT_FALSE(outScalarInfo->Has(vtkDataObject::FIELD_NUMBER_OF_TUPLES()));

    ASSERT_FALSE(outScalarInfo->Has(vtkDataObject::FIELD_NAME()));

    ASSERT_TRUE(outScalarInfo->Has(vtkDataObject::FIELD_ARRAY_TYPE()));
    ASSERT_EQ(inAttr->GetDataType(), outScalarInfo->Get(vtkDataObject::FIELD_ARRAY_TYPE()));

    auto outVectorInfo = vtkDataObject::GetActiveFieldInformation(getInfo->GetInInfo(),
        vtkDataObject::FIELD_ASSOCIATION_POINTS,
        vtkDataSetAttributes::VECTORS);

    ASSERT_TRUE(outVectorInfo);

    ASSERT_TRUE(outVectorInfo->Has(vtkDataObject::FIELD_NUMBER_OF_COMPONENTS()));
    ASSERT_EQ(inAttr->GetNumberOfComponents(), outVectorInfo->Get(vtkDataObject::FIELD_NUMBER_OF_COMPONENTS()));

    ASSERT_TRUE(outVectorInfo->Has(vtkDataObject::FIELD_NUMBER_OF_TUPLES()));
    ASSERT_EQ(inAttr->GetNumberOfTuples(), outVectorInfo->Get(vtkDataObject::FIELD_NUMBER_OF_TUPLES()));

    ASSERT_TRUE(outVectorInfo->Has(vtkDataObject::FIELD_NAME()));
    ASSERT_STREQ(inAttr->GetName(), outVectorInfo->Get(vtkDataObject::FIELD_NAME()));

    ASSERT_TRUE(outVectorInfo->Has(vtkDataObject::FIELD_ARRAY_TYPE()));
    ASSERT_EQ(inAttr->GetDataType(), outVectorInfo->Get(vtkDataObject::FIELD_ARRAY_TYPE()));
}

TEST_F(ArrayChangeInformationFilter_test, CheckWhichInformation_vtkAssignAttribute_x2_passes)
{
    inAttr->SetNumberOfComponents(3);
    inAttr->SetNumberOfTuples(17);
    auto inImage = vtkSmartPointer<vtkImageData>::New();
    inImage->GetPointData()->SetScalars(inAttr);
    inImage->SetExtent(0, 0, 2, 18, 3, 3);

    auto infoSource = vtkSmartPointer<InformationSource>::New();
    infoSource->SetOutput(inImage);

    vtkDataObject::SetActiveAttributeInfo(infoSource->GetOutInfo(),
        vtkDataObject::FIELD_ASSOCIATION_POINTS,
        vtkDataSetAttributes::SCALARS,
        inAttr->GetName(),
        inAttr->GetDataType(),
        inAttr->GetNumberOfComponents(),
        static_cast<int>(inAttr->GetNumberOfTuples()));

    filter->SetAttributeLocation(ArrayChangeInformationFilter::POINT_DATA);
    filter->SetAttributeType(vtkDataSetAttributes::SCALARS);
    filter->SetInputConnection(infoSource->GetOutputPort());
    filter->EnableRenameOff();
    filter->EnableSetUnitOff();

    auto assignToVectors = vtkSmartPointer<vtkAssignAttribute>::New();
    assignToVectors->SetInputConnection(filter->GetOutputPort());
    assignToVectors->Assign(vtkDataSetAttributes::SCALARS, vtkDataSetAttributes::VECTORS,
        vtkAssignAttribute::POINT_DATA);

    auto reassignToScalars = vtkSmartPointer<vtkAssignAttribute>::New();
    reassignToScalars->SetInputConnection(assignToVectors->GetOutputPort());
    reassignToScalars->Assign(vtkDataSetAttributes::VECTORS, vtkDataSetAttributes::SCALARS,
        vtkAssignAttribute::POINT_DATA);

    auto getInfo = vtkSmartPointer<InformationSink>::New();
    getInfo->SetInputConnection(reassignToScalars->GetOutputPort());

    getInfo->UpdateInformation();

    auto outScalarInfo = vtkDataObject::GetActiveFieldInformation(getInfo->GetInInfo(),
        vtkDataObject::FIELD_ASSOCIATION_POINTS,
        vtkDataSetAttributes::SCALARS);

    ASSERT_TRUE(outScalarInfo);

    ASSERT_TRUE(outScalarInfo->Has(vtkDataObject::FIELD_NUMBER_OF_COMPONENTS()));
    ASSERT_EQ(inAttr->GetNumberOfComponents(), outScalarInfo->Get(vtkDataObject::FIELD_NUMBER_OF_COMPONENTS()));

    ASSERT_TRUE(outScalarInfo->Has(vtkDataObject::FIELD_NUMBER_OF_TUPLES()));
    ASSERT_EQ(inAttr->GetNumberOfTuples(), outScalarInfo->Get(vtkDataObject::FIELD_NUMBER_OF_TUPLES()));

    ASSERT_TRUE(outScalarInfo->Has(vtkDataObject::FIELD_NAME()));
    ASSERT_STREQ(inAttr->GetName(), outScalarInfo->Get(vtkDataObject::FIELD_NAME()));

    ASSERT_TRUE(outScalarInfo->Has(vtkDataObject::FIELD_ARRAY_TYPE()));
    ASSERT_EQ(inAttr->GetDataType(), outScalarInfo->Get(vtkDataObject::FIELD_ARRAY_TYPE()));

    auto outVectorInfo = vtkDataObject::GetActiveFieldInformation(getInfo->GetInInfo(),
        vtkDataObject::FIELD_ASSOCIATION_POINTS,
        vtkDataSetAttributes::VECTORS);

    ASSERT_FALSE(outVectorInfo);
}

TEST_F(ArrayChangeInformationFilter_test, vtkAssignAttribute_CorrectNumberOfComponentsPassedDownstream)
{
    inAttr->SetNumberOfComponents(3);
    inAttr->SetNumberOfTuples(17);
    for (vtkIdType i = 0; i < inAttr->GetNumberOfValues(); ++i)
    {
        inAttr->SetValue(i, static_cast<float>(i));
    }
    auto inImage = vtkSmartPointer<vtkImageData>::New();
    inImage->GetPointData()->SetScalars(inAttr);
    inImage->SetExtent(0, 0, 2, 18, 3, 3);

    auto infoSource = vtkSmartPointer<InformationSource>::New();
    infoSource->SetOutput(inImage);

    vtkDataObject::SetActiveAttributeInfo(infoSource->GetOutInfo(),
        vtkDataObject::FIELD_ASSOCIATION_POINTS,
        vtkDataSetAttributes::SCALARS,
        inAttr->GetName(),
        inAttr->GetDataType(),
        inAttr->GetNumberOfComponents(),
        static_cast<int>(inAttr->GetNumberOfTuples()));

    filter->SetAttributeLocation(ArrayChangeInformationFilter::POINT_DATA);
    filter->SetAttributeType(vtkDataSetAttributes::SCALARS);
    filter->SetInputConnection(infoSource->GetOutputPort());
    filter->EnableRenameOff();
    filter->EnableSetUnitOff();

    auto assignToVectors = vtkSmartPointer<vtkAssignAttribute>::New();
    assignToVectors->SetInputConnection(filter->GetOutputPort());
    assignToVectors->Assign(vtkDataSetAttributes::SCALARS, vtkDataSetAttributes::VECTORS,
        vtkAssignAttribute::POINT_DATA);

    auto reassignScalars = vtkSmartPointer<vtkAssignAttribute>::New();
    reassignScalars->SetInputConnection(assignToVectors->GetOutputPort());
    reassignScalars->Assign(vtkDataSetAttributes::VECTORS, vtkDataSetAttributes::SCALARS,
        vtkAssignAttribute::POINT_DATA);

    auto normalize = vtkSmartPointer<vtkImageNormalize>::New();
    normalize->SetInputConnection(reassignScalars->GetOutputPort());
    normalize->SetEnableSMP(false);
    normalize->SetNumberOfThreads(1);

    // before patching (VTK 7.1+) vtkAssignAttribute, this would cause segmentation faults
    normalize->Update();
    auto outImg = normalize->GetOutput();
    auto outScalars = outImg->GetPointData()->GetScalars();

    ASSERT_EQ(inAttr->GetNumberOfComponents(), outScalars->GetNumberOfComponents());
    ASSERT_EQ(inAttr->GetNumberOfTuples(), outScalars->GetNumberOfTuples());
}
