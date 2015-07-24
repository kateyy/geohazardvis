#include <cassert>

#include <QDebug>

#include <vtkPolyData.h>
#include <vtkFloatArray.h>
#include <vtkCellData.h>
#include <vtkCellArray.h>

#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkProperty.h>
#include <vtkDelaunay2D.h>
#include <vtkPointData.h>
#include <vtkProbeFilter.h>
#include <vtkTransform.h>
#include <vtkTransformFilter.h>
#include <vtkWarpScalar.h>
#include <vtkAssignAttribute.h>
#include <vtkImageData.h>
#include <vtkBoundingBox.h>
#include <vtkVector.h>
#include <vtkMath.h>
#include <vtkLookupTable.h>
#include <vtkPassThrough.h>
#include <vtkTrivialProducer.h>
#include <vtkPointDataToCellData.h>

#include <core/data_objects/PolyDataObject.h>
#include <core/data_objects/ImageDataObject.h>
#include <core/io/Exporter.h>
#include <core/io/FileParser.h>
#include <core/io/Loader.h>
#include <core/io/MatricesToVtk.h>
#include <core/io/TextFileReader.h>
#include <core/rendered_data/RenderedData.h>
#include <core/color_mapping/ColorMapping.h>

#include <threadingzeug/parallelfor.h>



template<typename T, size_t Dimensions = 3>
class DataExtent
{
public:
    DataExtent()
    {
        for (size_t i = 0; i < Dimensions; ++i)
        {
            m_extent[2 * i] = std::numeric_limits<T>::max();
            m_extent[2 * i + 1] = std::numeric_limits<T>::lowest();
        }
    }
    DataExtent(T extent[Dimensions * 2])
    {
        std::copy(extent, extent + Dimensions * 2, m_extent);
    }

    void Add(const T other[Dimensions * 2])
    {
        for (int i = 0; i < Dimensions; ++i)
        {
            if (m_extents[2 * i] < other[2 * i])
                m_extents[2 * i] = other[2 * i];

            if (m_extents[2 * i + 1] > other[2 * i + 1])
                m_extents[2 * i + 1] = other[2 * i + 1];
        }
    }

    void Add(const DataExtent & other)
    {
        Add(other.m_extent);
    }

    T operator[] (size_t index) const
    {
        assert(index < Dimensions * 2);
        return m_extent[index];
    }

    T & operator[] (size_t index)
    {
        assert(index < Dimensions * 2);
        return m_extent[index];
    }

    bool operator==(const T other[Dimensions * 2]) const
    {
        for (size_t i = 0; i < Dimensions * 2; ++i)
        {
            if (m_extent[i] != other[i])
                return false;
        }
        return true;
    }

    bool operator==(const DataExtent & other) const
    {
        return operator==(other.m_extent);
    }

    const T * Data() const
    {
        return m_extent;
    }

    // unsafe workaround for non-const VTK interfaces (e.g., vtkImageData)
    T * Data()
    {
        return m_extent;
    }

private:
    T m_extent[Dimensions * 2];
};

using ImageExtent = DataExtent<int, 3>;


vtkSmartPointer<vtkDataArray> projectToLineOfSight(vtkDataArray & vectors, vtkVector3d lineOfSight)
{
    auto output = vtkSmartPointer<vtkDataArray>::Take(vectors.NewInstance());
    output->SetNumberOfComponents(1);
    output->SetNumberOfTuples(vectors.GetNumberOfTuples());

    lineOfSight.Normalize();

    auto projection = [output, &vectors, &lineOfSight](int i) {
        double scalarProjection = vtkMath::Dot(vectors.GetTuple(i), lineOfSight.GetData());
        output->SetTuple(i, &scalarProjection);
    };

    threadingzeug::sequential_for(0, output->GetNumberOfTuples(), projection);

    return output;
}


vtkSmartPointer<vtkDataArray> interpolateImageOnImage(vtkImageData & baseImage, vtkImageData & sourceImage, const QString & sourceAttributeName)
{
    bool structuralMatch =
        ImageExtent(baseImage.GetExtent()) == ImageExtent(sourceImage.GetExtent())
        && vtkVector3d(baseImage.GetOrigin()) == vtkVector3d(sourceImage.GetOrigin())
        && vtkVector3d(baseImage.GetSpacing()) == vtkVector3d(sourceImage.GetSpacing());

    if (structuralMatch)
    {
        if (sourceAttributeName.isEmpty())
        {
            return sourceImage.GetPointData()->GetScalars();
        }
        else
        {
            return sourceImage.GetPointData()->GetArray(sourceAttributeName.toUtf8().data());
        }
    }
    else
    {
        // TODO 
        // this requires a bit more work for images, the probe filter expects matching bounds/extents(?)
        return nullptr;
    }
}


/** Interpolate the source's attributes to the structure of the base data set */
vtkSmartPointer<vtkDataArray> interpolate(vtkDataSet & baseDataSet, vtkDataSet & sourceDataSet, const QString & sourceAttributeName, bool attributeInCellData)
{
    auto baseImage = vtkImageData::SafeDownCast(&baseDataSet);
    auto sourceImage = vtkImageData::SafeDownCast(&sourceDataSet);

    if (baseImage && sourceImage)
    {
        return interpolateImageOnImage(*baseImage, *sourceImage, sourceAttributeName);
    }

    auto basePoly = vtkPolyData::SafeDownCast(&baseDataSet);
    auto sourcePoly = vtkPolyData::SafeDownCast(&sourceDataSet);

    // interpolation: image <-> polygonal data

    auto baseDataProducer = vtkSmartPointer<vtkTrivialProducer>::New();
    baseDataProducer->SetOutput(&baseDataSet);
    auto sourceDataProducer = vtkSmartPointer<vtkTrivialProducer>::New();
    sourceDataProducer->SetOutput(&sourceDataSet);

    vtkSmartPointer<vtkAlgorithm> baseDataFilter = baseDataProducer;
    vtkSmartPointer<vtkAlgorithm> sourceDataFilter = sourceDataProducer;

    // assign source data scalars, if required
    if (!sourceAttributeName.isEmpty())
    {
        auto assign = vtkSmartPointer<vtkAssignAttribute>::New();
        assign->Assign(sourceAttributeName.toUtf8().data(), vtkDataSetAttributes::SCALARS,
            attributeInCellData ? vtkAssignAttribute::CELL_DATA : vtkAssignAttribute::POINT_DATA);
        assign->SetInputConnection(sourceDataFilter->GetOutputPort());
        sourceDataFilter = assign;
    }

    // flatten polygonal data before interpolation

    auto createFlattener = [](vtkAlgorithm * upstream) -> vtkSmartPointer<vtkAlgorithm>
    {
        auto flattenerTransform = vtkSmartPointer<vtkTransform>::New();
        flattenerTransform->Scale(1, 1, 0);
        auto flattener = vtkSmartPointer<vtkTransformFilter>::New();
        flattener->SetTransform(flattenerTransform);
        flattener->SetInputConnection(upstream->GetOutputPort());

        return flattener;
    };

    if (basePoly)
    {
        baseDataFilter = createFlattener(baseDataFilter);
    }
    if (sourcePoly)
    {
        sourceDataFilter = createFlattener(sourceDataFilter);
    }


    // now probe: at least on of the data sets is a polygonal one here

    auto probe = vtkSmartPointer<vtkProbeFilter>::New();
    probe->SetInputConnection(baseDataFilter->GetOutputPort());
    probe->SetSourceConnection(sourceDataFilter->GetOutputPort());

    vtkSmartPointer<vtkAlgorithm> resultAlgorithm = probe;

    // the probe creates point based values for polygons, but we want them associated with the centroids
    if (basePoly)
    {
        auto pointToCellData = vtkSmartPointer<vtkPointDataToCellData>::New();
        pointToCellData->SetInputConnection(probe->GetOutputPort());
        resultAlgorithm = pointToCellData;
    }

    resultAlgorithm->Update();

    qDebug() << "Points: " << baseDataSet.GetNumberOfPoints();
    qDebug() << "Valid points: " << probe->GetValidPoints()->GetNumberOfTuples();

    vtkSmartPointer<vtkDataSet> probedDataSet = vtkDataSet::SafeDownCast(resultAlgorithm->GetOutputDataObject(0));

    if (basePoly)
    {
        if (sourceAttributeName.isEmpty())
            return probedDataSet->GetCellData()->GetScalars();
        else
            return probedDataSet->GetCellData()->GetArray(sourceAttributeName.toUtf8().data());
    }

    if (sourceAttributeName.isEmpty())
        return probedDataSet->GetPointData()->GetScalars();
    else
        return probedDataSet->GetPointData()->GetArray(sourceAttributeName.toUtf8().data());
}


int main()
{
    QString inSARImg("C:/develop/$sync/GFZ/data/VTK XML data/InSAR displacements (cm).vti");
    QString firstInSARImg("C:/develop/$sync/GFZ/data/VTK XML data/displacements.vti");
    QString modelImg("C:/develop/$sync/GFZ/data/VTK XML data/Modeled Displacements (cm).vti");
    QString volcR10("C:/develop/$sync/GFZ/data/VTK XML data/Volcano 2 topo.vtp");
    QString volcModel("C:/develop/$sync/GFZ/data/simulated/Topography Model innerTDs.vtp");


    // ****** input parameters ******

    vtkVector3d lineOfSight(0, 0, 1);
    double observationUnitFactor = 1;
    double modelUnitFactor = 10e6;
    //double modelUnitFactor = 1;

    const auto & observationFN = inSARImg;
    const auto & modelFN = volcR10;

    auto observationObject = Loader::readFile(observationFN);
    auto modelObject = Loader::readFile(modelFN);

    bool interpolateModelOnObservation = false;

    QString observationAttributeName;
    bool useObservationCellData = false;
    QString modelAttributeName;
    bool useModelCellData = false;


    assert(observationObject->dataSet());
    auto & observationDataSet = *observationObject->dataSet();
    assert(modelObject->dataSet());
    auto & modelDataSet = *modelObject->dataSet();
        
    // ***** a bit of magic to find the correct source data

    auto checkImageAttributeName = [](vtkDataSet & image) -> QString
    {
        if (auto scalars = image.GetPointData()->GetScalars())
        {
            if (auto name = scalars->GetName())
                return QString::fromUtf8(name);
        }
        else
        {
            if (auto firstArray = image.GetPointData()->GetArray(0))
            {
                if (auto name = firstArray->GetName())
                    return QString::fromUtf8(name);
            }
        }
        return{};
    };

    observationAttributeName = checkImageAttributeName(observationDataSet);

    if (auto poly = vtkPolyData::SafeDownCast(modelObject->dataSet()))
    {
        // assuming that we store attributes in polygonal data always per cell
        auto scalars = poly->GetCellData()->GetScalars();
        if (!scalars)
            scalars = poly->GetCellData()->GetArray("displacement vectors");
        if (!scalars)
            scalars = poly->GetCellData()->GetArray("U+");
        if (scalars)
        {
            auto name = scalars->GetName();
            if (name)
                modelAttributeName = QString::fromUtf8(name);
            qDebug() << modelAttributeName << scalars->GetRange()[0] << scalars->GetRange()[1];
        }

        useModelCellData = true;
    }
    else
    {
        modelAttributeName = checkImageAttributeName(modelDataSet);
        useModelCellData = false;
    }


    if (observationAttributeName.isEmpty() || modelAttributeName.isEmpty())
        return 1;


    vtkSmartPointer<vtkDataArray> observationData;
    vtkSmartPointer<vtkDataArray> modelData;

    if (interpolateModelOnObservation)
    {
        observationData = observationAttributeName.isEmpty()
            ? observationDataSet.GetPointData()->GetScalars()
            : observationDataSet.GetPointData()->GetArray(observationAttributeName.toUtf8().data());

        modelData = interpolate(observationDataSet, modelDataSet, modelAttributeName, useModelCellData);
    }
    else
    {
        observationData = interpolate(modelDataSet, observationDataSet, observationAttributeName, useObservationCellData);

        modelData = useModelCellData
            ? modelDataSet.GetCellData()->GetArray(modelAttributeName.toUtf8().data())
            : modelDataSet.GetPointData()->GetArray(modelAttributeName.toUtf8().data());
    }


    if (!observationData || !modelData)
        return 2;


    // project vectors if needed
    if (observationData->GetNumberOfComponents() == 3)
    {
        observationData = projectToLineOfSight(*observationData, lineOfSight);
    }
    if (modelData->GetNumberOfComponents() == 3)
    {
        modelData = projectToLineOfSight(*modelData, lineOfSight);
    }

    assert(modelData->GetNumberOfComponents() == 1);
    assert(observationData->GetNumberOfComponents() == 1);
    assert(modelData->GetNumberOfTuples() == observationData->GetNumberOfTuples());


    // compute the residual data

    auto & referenceDataSet = interpolateModelOnObservation
        ? observationDataSet
        : modelDataSet;

    auto residualData = vtkSmartPointer<vtkDataArray>::Take(modelData->NewInstance());
    residualData->SetName("Residual");
    residualData->SetNumberOfTuples(modelData->GetNumberOfTuples());
    residualData->SetNumberOfComponents(1);

    for (vtkIdType i = 0; i < residualData->GetNumberOfTuples(); ++i)
    {
        auto o_value = observationData->GetTuple(i)[0] * observationUnitFactor;
        auto m_value = modelData->GetTuple(i)[0] * modelUnitFactor;

        double r_value = o_value - m_value;
        residualData->SetTuple(i, &r_value);
    }

    qDebug() << observationData->GetRange()[0] << observationData->GetRange()[1];
    qDebug() << residualData->GetRange()[0] << residualData->GetRange()[1];


    std::unique_ptr<DataObject> residualObject;

    auto residual = vtkSmartPointer<vtkDataSet>::Take(referenceDataSet.NewInstance());
    residual->CopyStructure(&referenceDataSet);

    if (auto residualImage = vtkImageData::SafeDownCast(residual))
    {
        assert(residualData->GetNumberOfTuples() == residual->GetNumberOfPoints());
        qDebug() << residualData->GetName();

        residual->GetPointData()->SetScalars(residualData);

        residualObject = std::make_unique<ImageDataObject>("Residual", *residualImage);
    }
    else if (auto residualPoly = vtkPolyData::SafeDownCast(residual))
    {
        assert(vtkPolyData::SafeDownCast(residual));
        // assuming that we store attributes in polygonal data always per cell
        assert(residual->GetNumberOfCells() == residualData->GetNumberOfTuples());

        residual->GetCellData()->SetScalars(residualData);

        residualObject = std::make_unique<PolyDataObject>("Residual", *residualPoly);
    }
    else
    {
        return 3;
    }

    auto window = vtkSmartPointer<vtkRenderWindow>::New();
    auto interactor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
    interactor->SetRenderWindow(window);
    window->SetInteractor(interactor);

    auto ren = vtkSmartPointer<vtkRenderer>::New();
    ren->SetBackground(1, 1, 1);
    window->AddRenderer(ren);

    auto rendered = residualObject->createRendered();

    auto colorMapping = std::make_unique<ColorMapping>();

    auto lut = vtkSmartPointer<vtkLookupTable>::New();
    lut->Build();
    colorMapping->setGradient(lut);

    colorMapping->setVisualizedData({ rendered.get() });

    for (auto & name : colorMapping->scalarsNames())
    {
        if (name != "user-defined color")
        {
            colorMapping->setCurrentScalarsByName(name);
            break;
        }
    }

    rendered->viewProps()->InitTraversal();
    for (auto prop = rendered->viewProps()->GetNextProp(); prop; prop = rendered->viewProps()->GetNextProp())
    {
        ren->AddViewProp(prop);
    }

    ren->ResetCamera();

    interactor->Initialize();
    interactor->Start();

    return 0;
}
