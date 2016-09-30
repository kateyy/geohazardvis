#include <gtest/gtest.h>

#include <vector>

#include <vtkCamera.h>
#include <vtkImageData.h>
#include <vtkPointData.h>
#include <vtkSmartPointer.h>
#include <vtkUnsignedCharArray.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkVector.h>
#include <vtkWindowToImageFilter.h>

#include <core/color_mapping/ColorMapping.h>
#include <core/data_objects/ImageDataObject.h>
#include <core/rendered_data/RenderedImageData.h>


class ColorMapping_test : public testing::Test
{
public:
    static vtkSmartPointer<vtkUnsignedCharArray> createUCharColors()
    {
        auto colorData = std::vector<unsigned char>{ // r, g, b [0,255]; 2x2
            0, 1, 2,
            30, 31, 32,
            180, 181, 182,
            253, 254, 255
        };

        auto colorArray = vtkSmartPointer<vtkUnsignedCharArray>::New();
        colorArray->SetName("Colors");
        colorArray->SetNumberOfComponents(3);
        colorArray->SetNumberOfTuples(4);
        colorArray->SetArray(colorData.data(), static_cast<vtkIdType>(colorData.size()), true);

        return colorArray;
    }

    struct ImageStruct
    {
        std::unique_ptr<ImageDataObject> image;
        std::unique_ptr<RenderedImageData> rendered;
        QString scalarsName;

        ImageStruct() = default;

        ImageStruct(ImageStruct && other)
            : image{ std::move(other.image) }
            , rendered{ std::move(other.rendered) }
            , scalarsName{ std::move(other.scalarsName) }
        {
        }
    };

    static ImageStruct createImage(vtkUnsignedCharArray & colors)
    {
        auto imageData = vtkSmartPointer<vtkImageData>::New();
        imageData->SetExtent(0, 1, 0, 1, 0, 0);
        imageData->GetPointData()->SetScalars(&colors);

        ImageStruct img;

        img.image = std::make_unique<ImageDataObject>("Picture", *imageData);
        img.rendered.reset(
            dynamic_cast<RenderedImageData *>(img.image->createRendered().release()));

        const auto scalars = img.rendered->colorMapping().scalarsNames();
        const auto directColors = scalars.filter(QRegExp(".*direct colors.*", Qt::CaseInsensitive));
        img.scalarsName = directColors.isEmpty() ? "" : directColors.first();

        return img;
    }
};


TEST_F(ColorMapping_test, DirectImageColors_detects_uchar)
{
    auto colorArray = createUCharColors();
    auto img = createImage(*colorArray);

    ASSERT_FALSE(img.scalarsName.isEmpty());
}

TEST_F(ColorMapping_test, DirectImageColors_renders_uchar)
{
    auto colorArray = createUCharColors();
    auto img = createImage(*colorArray);

    img.rendered->colorMapping().setCurrentScalarsByName(img.scalarsName);
    img.rendered->setInterpolationEnabled(false);

    auto viewProps = img.rendered->viewProps();

    auto ren = vtkSmartPointer<vtkRenderer>::New();
    for (viewProps->InitTraversal(); auto prop = viewProps->GetNextProp(); )
    {
        ren->AddViewProp(prop);
    }

    auto renWin = vtkSmartPointer<vtkRenderWindow>::New();
    renWin->AddRenderer(ren);
    renWin->OffScreenRenderingOn();
    renWin->SetSize(2, 2);
    
    auto &cam = *ren->GetActiveCamera();
    cam.SetParallelProjection(true);
    ren->ResetCamera();
    cam.SetFocalPoint(0.5, 0.5, 0.0);
    cam.SetParallelScale(0.5);

    auto windowToImage = vtkSmartPointer<vtkWindowToImageFilter>::New();
    windowToImage->SetInput(renWin);
    windowToImage->SetInputBufferTypeToRGBA();
    windowToImage->ReadFrontBufferOff();
    windowToImage->Update();

    auto renderedImage = windowToImage->GetOutput();
    auto renderedColors = vtkUnsignedCharArray::SafeDownCast(renderedImage->GetPointData()->GetScalars());
    assert(renderedColors);

    ASSERT_EQ(colorArray->GetNumberOfTuples(), renderedColors->GetNumberOfTuples());

    for (int i = 0; i < colorArray->GetNumberOfTuples(); ++i)
    {
        vtkVector<unsigned char, 4> refRgba;
        colorArray->GetTypedTuple(i, refRgba.GetData());
        refRgba[3] = 0xFF;

        vtkVector<unsigned char, 4> testRgba;
        renderedColors->GetTypedTuple(i, testRgba.GetData());

        ASSERT_EQ(refRgba, testRgba);
    }
}
