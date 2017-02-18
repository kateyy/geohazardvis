#include <gtest/gtest.h>

#include <vtkAlgorithmOutput.h>
#include <vtkImageData.h>
#include <vtkInformation.h>
#include <vtkFloatArray.h>
#include <vtkPassThrough.h>
#include <vtkPointData.h>
#include <vtkSmartPointer.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkMultiBlockFromTimeSeriesFilter.h>

#include <vtkTemporalInterpolator.h>

#include <core/TemporalPipelineMediator.h>
#include <core/types.h>
#include <core/data_objects/ImageDataObject.h>
#include <core/filters/TemporalDataSource.h>
#include <core/rendered_data/RenderedImageData.h>
#include <core/table_model/QVtkTableModel.h>


class TemporalPipelineMediator_test : public ::testing::Test
{
public:

    static const std::vector<double> & timeSteps()
    {
        static const std::vector<double> ts = { 42.1, 42.2 };
        return ts;
    }

    static const char * attributeName()
    {
        static const char * const name = "temporalAttr";
        return name;
    }

    static const std::vector<float> & timeStepValues()
    {
        static const std::vector<float> ts = { 1.0f, 2.0f };
        return ts;
    }

    static vtkSmartPointer<vtkImageData> createImage()
    {
        auto image = vtkSmartPointer<vtkImageData>::New();
        image->SetDimensions(2, 1, 1);
        image->AllocateScalars(VTK_FLOAT, 1);
        return image;
    }

    static vtkSmartPointer<TemporalDataSource> createTemporalSource()
    {
        auto temporalSource = vtkSmartPointer<TemporalDataSource>::New();
        const auto id = temporalSource->AddTemporalAttribute(
            TemporalDataSource::AttributeLocation::POINT_DATA, attributeName());

        for (size_t i = 0; i < timeSteps().size(); ++i)
        {
            auto data = vtkSmartPointer<vtkFloatArray>::New();
            data->SetName(attributeName());
            data->SetNumberOfValues(2);
            data->SetValue(0, timeStepValues()[i]);
            data->SetValue(1, timeStepValues()[i]);

            temporalSource->SetTemporalAttributeTimeStep(
                TemporalDataSource::AttributeLocation::POINT_DATA, id,
                timeSteps()[i],
                data);
        }

        return temporalSource;
    }

    class TemporalData : public ImageDataObject
    {
    public:
        TemporalData()
            : ImageDataObject("Temporal Data", *createImage())
            , temporalSource{ createTemporalSource() }
        {
            temporalSource->SetInputConnection(DataObject::processedOutputPortInternal());
        }

        vtkSmartPointer<TemporalDataSource> temporalSource;

        vtkAlgorithmOutput * processedOutputPortInternal() override
        {
            return temporalSource->GetOutputPort();
        }
    };

};


TEST_F(TemporalPipelineMediator_test, SelectTimeStepByIndex_subclassed)
{
    TemporalData data;
    auto rendered = data.createRendered();
    TemporalPipelineMediator mediator;
    mediator.setVisualization(rendered.get());
    const auto index = 1u;
    mediator.selectTimeStepByIndex(index);
    ASSERT_EQ(timeSteps()[index], mediator.selectedTimeStep());

    auto processedDataSet = rendered->processedOutputDataSet();
    ASSERT_TRUE(processedDataSet);
    rendered->processedOutputPort()->GetProducer()->Update();
    auto timeStepExtractor = rendered->processedOutputPort()->GetProducer(); // this is quite internal...
    ASSERT_TRUE(timeStepExtractor);
    auto upstreamAlgorithm = timeStepExtractor->GetInputAlgorithm();
    ASSERT_TRUE(upstreamAlgorithm);
    auto & outInfo = *upstreamAlgorithm->GetOutputInformation(0);

    ASSERT_TRUE(outInfo.Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()));
    ASSERT_EQ(timeSteps()[index], outInfo.Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()));

    auto attribute = processedDataSet->GetPointData()->GetArray(attributeName());
    ASSERT_TRUE(attribute);
    ASSERT_EQ(2, attribute->GetNumberOfValues());
    ASSERT_EQ(timeStepValues()[index], static_cast<float>(attribute->GetComponent(0, 0)));
}

TEST_F(TemporalPipelineMediator_test, SelectTimeStepByIndex_injected)
{
    ImageDataObject data("Temporal Data", *createImage());
    auto temporalDataSource = createTemporalSource();
    data.injectPostProcessingStep({ temporalDataSource, temporalDataSource });

    auto rendered = data.createRendered();
    TemporalPipelineMediator mediator;
    mediator.setVisualization(rendered.get());
    const auto index = 1u;
    mediator.selectTimeStepByIndex(index);
    ASSERT_EQ(timeSteps()[index], mediator.selectedTimeStep());

    auto processedDataSet = rendered->processedOutputDataSet();
    ASSERT_TRUE(processedDataSet);
    rendered->processedOutputPort()->GetProducer()->Update();
    auto timeStepExtractor = rendered->processedOutputPort()->GetProducer(); // this is quite internal...
    ASSERT_TRUE(timeStepExtractor);
    auto upstreamAlgorithm = timeStepExtractor->GetInputAlgorithm();
    ASSERT_TRUE(upstreamAlgorithm);
    auto & outInfo = *upstreamAlgorithm->GetOutputInformation(0);

    ASSERT_TRUE(outInfo.Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()));
    ASSERT_EQ(timeSteps()[index], outInfo.Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()));

    auto attribute = processedDataSet->GetPointData()->GetArray(attributeName());
    ASSERT_TRUE(attribute);
    ASSERT_EQ(2, attribute->GetNumberOfValues());
    ASSERT_EQ(timeStepValues()[index], static_cast<float>(attribute->GetComponent(0, 0)));
}

TEST_F(TemporalPipelineMediator_test, UpdateDownStreamAlgorithm)
{
    ImageDataObject data("Temporal Data", *createImage());
    auto temporalDataSource = createTemporalSource();
    data.injectPostProcessingStep({ temporalDataSource, temporalDataSource });

    auto rendered = data.createRendered();
    TemporalPipelineMediator mediator;
    mediator.setVisualization(rendered.get());

    auto downStreamAlgorithm = vtkSmartPointer<vtkPassThrough>::New();
    downStreamAlgorithm->SetInputConnection(rendered->processedOutputPort());

    // Make sure the whole pipeline is up to date with the initial settings.
    ASSERT_TRUE(downStreamAlgorithm->GetExecutive()->Update());

    // Now this should request downstream updates.
    const auto index = 1u;
    mediator.selectTimeStepByIndex(index);

    ASSERT_TRUE(downStreamAlgorithm->GetExecutive()->Update());

    auto output = vtkDataSet::SafeDownCast(downStreamAlgorithm->GetOutput());
    ASSERT_TRUE(output);
    auto scalars = vtkFloatArray::FastDownCast(output->GetPointData()->GetAbstractArray(attributeName()));
    ASSERT_TRUE(scalars);

    ASSERT_EQ(timeSteps()[index], scalars->GetInformation()->Get(vtkDataObject::DATA_TIME_STEP()));
    ASSERT_EQ(timeStepValues()[index], scalars->GetValue(0));
}
