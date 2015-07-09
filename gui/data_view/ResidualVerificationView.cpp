#include <gui/data_view/ResidualVerificationView.h>

#include <cassert>
#include <functional>
#include <random>

#include <QBoxLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QDebug>
#include <QDoubleSpinBox>
#include <QToolBar>

#include <QVTKWidget.h>
#include <vtkAssignAttribute.h>
#include <vtkCellData.h>
#include <vtkElevationFilter.h>
#include <vtkFloatArray.h>
#include <vtkImageData.h>
#include <vtkMath.h>
#include <vtkPointData.h>
#include <vtkPointDataToCellData.h>
#include <vtkPolyData.h>
#include <vtkProbeFilter.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkTransform.h>
#include <vtkTransformFilter.h>
#include <vtkWarpScalar.h>

#include <threadingzeug/parallelfor.h>

#include <core/AbstractVisualizedData.h>
#include <core/DataSetHandler.h>
#include <core/types.h>
#include <core/color_mapping/ColorMapping.h>
#include <core/data_objects/ImageDataObject.h>
#include <core/data_objects/PolyDataObject.h>
#include <gui/SelectionHandler.h>
#include <gui/data_view/RendererImplementationBase3D.h>
#include <gui/data_view/RenderViewStrategyImage2D.h>
#include <gui/data_view/RenderViewStrategy3D.h>


namespace
{

vtkSmartPointer<vtkDataArray> surfaceVectorsToInSAR(vtkDataArray & vectors, vtkVector3d lineOfSight)
{
    auto output = vtkSmartPointer<vtkDataArray>::Take(vectors.NewInstance());
    output->SetNumberOfComponents(1);
    output->SetNumberOfTuples(vectors.GetNumberOfTuples());

    lineOfSight.Normalize();

    auto projection = [output, &vectors, &lineOfSight] (int i) {
        double scalarProjection = vtkMath::Dot(vectors.GetTuple(i), lineOfSight.GetData());
        output->SetTuple(i, &scalarProjection);
    };

    threadingzeug::parallel_for(0, output->GetNumberOfTuples(), projection);

    return output;
}

vtkSmartPointer<vtkImageData> createImageFromPoly(vtkImageData & referenceGrid, vtkPolyData & poly)
{
    // this will set the elevation as active scalars!
    auto polyElevationScalars = vtkSmartPointer<vtkElevationFilter>::New();
    polyElevationScalars->SetInputData(&poly);

    auto polyFlattenerTransform = vtkSmartPointer<vtkTransform>::New();
    polyFlattenerTransform->Scale(1, 1, 0);
    auto polyFlattener = vtkSmartPointer<vtkTransformFilter>::New();
    polyFlattener->SetTransform(polyFlattenerTransform);
    polyFlattener->SetInputConnection(polyElevationScalars->GetOutputPort());

    // create new data set, to not pass the referenceGrid's attributes to the output
    auto newGridStructure = vtkSmartPointer<vtkImageData>::New();
    newGridStructure->CopyStructure(&referenceGrid);

    auto probe = vtkSmartPointer<vtkProbeFilter>::New();
    probe->SetInputData(newGridStructure);
    probe->SetSourceConnection(polyFlattener->GetOutputPort());

    probe->Update();
    vtkSmartPointer<vtkImageData> newImage = vtkImageData::SafeDownCast(probe->GetOutput());

    return newImage;
}

vtkSmartPointer<vtkPolyData> interpolateImageOnPoly(vtkPolyData & referencePoly, vtkImageData & image)
{
    auto newPolyStructure = vtkSmartPointer<vtkPolyData>::New();
    // better using DeepCopy here?
    newPolyStructure->ShallowCopy(&referencePoly);

    auto polyElevationScalars = vtkSmartPointer<vtkElevationFilter>::New();
    polyElevationScalars->SetInputData(newPolyStructure);

    auto polyFlattenerTransform = vtkSmartPointer<vtkTransform>::New();
    polyFlattenerTransform->Scale(1, 1, 0);
    auto polyFlattener = vtkSmartPointer<vtkTransformFilter>::New();
    polyFlattener->SetTransform(polyFlattenerTransform);
    polyFlattener->SetInputConnection(polyElevationScalars->GetOutputPort());

    // interpolate image data to the flattened surface
    auto probe = vtkSmartPointer<vtkProbeFilter>::New();
    probe->SetInputConnection(polyFlattener->GetOutputPort());
    probe->SetSourceData(&image);

    // restore the elevation
    auto elevate = vtkSmartPointer<vtkWarpScalar>::New();
    elevate->UseNormalOn();
    elevate->SetNormal(0, 0, 1);
    elevate->SetInputConnection(probe->GetOutputPort());
    elevate->Update();

    // vtkProbeFilter interpolates at point positions, we need cell attributes
    // alternatively, probe on referencePoly's centroids to omit the additional interpolation step
    auto pointToCellData = vtkSmartPointer<vtkPointDataToCellData>::New();
    pointToCellData->SetInputConnection(elevate->GetOutputPort());
    pointToCellData->Update();

    vtkSmartPointer<vtkPolyData> outputPoly = vtkPolyData::SafeDownCast(pointToCellData->GetOutput());
    return outputPoly;
}

}


ResidualVerificationView::ResidualVerificationView(int index, QWidget * parent, Qt::WindowFlags flags)
    : AbstractRenderView(index, parent, flags)
    , m_qvtkMain(nullptr)
    , m_observationCombo(nullptr)
    , m_modelCombo(nullptr)
    , m_inSARLineOfSight(0, 0, 1)
    , m_interpolateModelOnObservation(false)
    , m_implementation(nullptr)
    , m_strategy(nullptr)
{
    auto layout = new QBoxLayout(QBoxLayout::Direction::TopToBottom);
    layout->setMargin(0);
    layout->setSpacing(0);

    m_qvtkMain = new QVTKWidget(this);
    m_qvtkMain->setMinimumSize(300, 300);
    layout->addWidget(m_qvtkMain);

    setLayout(layout);

    auto interpolationSwitch = new QCheckBox();
    interpolationSwitch->setChecked(m_interpolateModelOnObservation);
    // TODO correctly link in both ways
    connect(interpolationSwitch, &QAbstractButton::toggled, this, &ResidualVerificationView::setInterpolateModelOnObservation);
    toolBar()->addWidget(interpolationSwitch);

    m_observationCombo = new QComboBox();
    m_modelCombo = new QComboBox();
    toolBar()->addWidget(m_observationCombo);
    toolBar()->addWidget(m_modelCombo);

    for (int i = 0; i < 3; ++i)
    {
        auto losEdit = new QDoubleSpinBox();
        losEdit->setRange(0, 1);
        losEdit->setSingleStep(0.01);
        losEdit->setValue(m_inSARLineOfSight[i]);
        connect(losEdit, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), [this, i, losEdit] (double val) {
            m_inSARLineOfSight[i] = val;
        });
        connect(this, &ResidualVerificationView::lineOfSightChanged, [losEdit, i] (const vtkVector3d & los) {
            losEdit->setValue(los[i]);
        });
        toolBar()->addWidget(losEdit);
    }

    initialize();   // lazy initialize in not really needed for now

    SelectionHandler::instance().addRenderView(this);

    connect(&DataSetHandler::instance(), &DataSetHandler::dataObjectsChanged, this, &ResidualVerificationView::updateComboBoxes);

    connect(m_observationCombo, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
        this, &ResidualVerificationView::updateObservationFromUi);
    connect(m_modelCombo, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
        this, &ResidualVerificationView::updateModelFromUi);

    updateComboBoxes();
}

ResidualVerificationView::~ResidualVerificationView()
{
    SelectionHandler::instance().removeRenderView(this);

    for (auto & vis : m_visualizations)
    {
        if (!vis)
            continue;

        beforeDeleteVisualization(vis.get());
        vis.release();
    }

    if (m_implementation)
    {
        m_implementation->deactivate(m_qvtkMain);
        delete m_implementation;
    }
}

QString ResidualVerificationView::friendlyName() const
{
    return "Observation, Model, Residual";
}

ContentType ResidualVerificationView::contentType() const
{
    return ContentType::Rendered2D;
}

DataObject * ResidualVerificationView::selectedData() const
{
    return m_implementation->selectedData();
}

AbstractVisualizedData * ResidualVerificationView::selectedDataVisualization() const
{
    auto data = selectedData();
    if (!data)
        return nullptr;

    for (auto && vis : m_visualizations)
    {
        if (vis && &vis->dataObject() == data)
            return vis.get();
    }

    return nullptr;
}

void ResidualVerificationView::lookAtData(DataObject * dataObject, vtkIdType itemId)
{
    m_implementation->lookAtData(dataObject, itemId);
}

AbstractVisualizedData * ResidualVerificationView::visualizationFor(DataObject * dataObject, int subViewIndex) const
{
    if (subViewIndex == -1)
    {
        for (size_t i = 0; i < m_dataSets.size(); ++i)
        {
            if (m_dataSets[i] == dataObject && m_visualizations.size() > i)
                return m_visualizations[i].get();
        }
        return nullptr;
    }

    assert(subViewIndex >= 0);
    assert(m_dataSets.size() >= m_visualizations.size());
    if (m_dataSets[subViewIndex] != dataObject || m_visualizations.size() < static_cast<size_t>(subViewIndex))
        return nullptr;

    return m_visualizations[subViewIndex].get();
}

void ResidualVerificationView::setObservationData(ImageDataObject * observation)
{
    setDataHelper(0, observation);
}

void ResidualVerificationView::setModelData(DataObject * model)
{
    setDataHelper(1, model);
}

void ResidualVerificationView::setResidualData(DataObject * residual)
{
    setDataHelper(2, residual);
}

void ResidualVerificationView::setInSARLineOfSight(const vtkVector3d & los)
{
    m_inSARLineOfSight = los;

    emit lineOfSightChanged(los);
}

const vtkVector3d & ResidualVerificationView::inSARLineOfSight() const
{
    return m_inSARLineOfSight;
}

void ResidualVerificationView::setInterpolateModelOnObservation(bool modelToObservation)
{
    m_interpolateModelOnObservation = modelToObservation;
}

bool ResidualVerificationView::interpolatemodelOnObservation() const
{
    return m_interpolateModelOnObservation;
}

void ResidualVerificationView::setDataHelper(unsigned int subViewIndex, DataObject * data, bool skipGuiUpdate, std::vector<std::unique_ptr<AbstractVisualizedData>> * toDelete)
{
    assert(skipGuiUpdate == (toDelete != nullptr));

    if (m_dataSets.size() > subViewIndex && m_dataSets[subViewIndex] == data)
        return;

    std::vector<std::unique_ptr<AbstractVisualizedData>> toDeleteInternal;
    setDataInternal(subViewIndex, data, toDeleteInternal);

    // create a residual only if we didn't just set one
    if (subViewIndex != 2 || !data)
        updateResidual(toDeleteInternal);

    // update GUI before actually deleting old visualization data

    if (skipGuiUpdate)
    {
        for (auto & it : toDeleteInternal)
            toDelete->push_back(std::move(it));
    }
    else
    {
        updateGuiAfterDataChange();
    }
}

unsigned int ResidualVerificationView::numberOfSubViews() const
{
    return 3;
}

vtkRenderWindow * ResidualVerificationView::renderWindow()
{
    assert(m_qvtkMain);
    return m_qvtkMain->GetRenderWindow();
}

const vtkRenderWindow * ResidualVerificationView::renderWindow() const
{
    assert(m_qvtkMain);
    return m_qvtkMain->GetRenderWindow();
}

RendererImplementation & ResidualVerificationView::implementation() const
{
    assert(m_implementation);
    return *m_implementation;
}

void ResidualVerificationView::render()
{
    if (!isVisible())
        return;

    assert(m_qvtkMain);
    m_qvtkMain->GetRenderWindow()->Render();
}

void ResidualVerificationView::showEvent(QShowEvent * event)
{
    AbstractDataView::showEvent(event);

    initialize();
}

QWidget * ResidualVerificationView::contentWidget()
{
    return m_qvtkMain;
}

void ResidualVerificationView::highlightedIdChangedEvent(DataObject * dataObject, vtkIdType itemId)
{
    m_implementation->setSelectedData(dataObject, itemId);
}

void ResidualVerificationView::showDataObjectsImpl(const QList<DataObject *> & dataObjects,
    QList<DataObject *> & incompatibleObjects,
    unsigned int subViewIndex)
{
    if (dataObjects.size() > 1)
        qDebug() << "Multiple objects per sub-view not supported in the ResidualVerificationView.";

    for (int i = 1; i < dataObjects.size(); ++i)
        incompatibleObjects << dataObjects[i];

    DataObject * data = dataObjects.isEmpty() ? nullptr : dataObjects.first();
    auto imageData = dynamic_cast<ImageDataObject *>(data);
    if (!imageData)
    {
        qDebug() << "ResidualVerificationView only supports ImageDataObjects!";
        incompatibleObjects.prepend(data);
        return;
    }

    assert(m_dataSets.size() < subViewIndex || m_dataSets[subViewIndex] == nullptr || m_dataSets[subViewIndex] == imageData);

    setDataHelper(subViewIndex, imageData);
}

void ResidualVerificationView::hideDataObjectsImpl(const QList<DataObject *> & dataObjects, unsigned int subViewIndex)
{
    assert(unsigned(m_dataSets.size()) > subViewIndex);

    // no caching for now, just remove the object
    if (dataObjects.contains(m_dataSets[subViewIndex]))
        setDataHelper(subViewIndex, nullptr);
}

QList<DataObject *> ResidualVerificationView::dataObjectsImpl(int subViewIndex) const
{
    if (subViewIndex == -1)
    {
        QList<DataObject *> objects;

        for (auto && image : m_dataSets)
            if (image)
                objects << image;

        return objects;
    }

    if (m_dataSets.size() > static_cast<size_t>(subViewIndex) && m_dataSets[subViewIndex])
        return{ m_dataSets[subViewIndex] };
        
    return{};
}

void ResidualVerificationView::prepareDeleteDataImpl(const QList<DataObject *> & dataObjects)
{
    std::vector<std::unique_ptr<AbstractVisualizedData>> toDelete;

    for (auto objectToDelete : dataObjects)
    {
        for (size_t i = 0; i < m_dataSets.size(); ++i)
        {
            if (objectToDelete == m_dataSets[i])
                setDataHelper(static_cast<unsigned int>(i), nullptr, true, &toDelete);
        }
    }

    updateGuiAfterDataChange();
}

QList<AbstractVisualizedData *> ResidualVerificationView::visualizationsImpl(int subViewIndex) const
{
    QList<AbstractVisualizedData *> validVis;

    if (subViewIndex == -1)
    {
        for (auto & vis : m_visualizations)
            if (vis)
                validVis << vis.get();
        return validVis;
    }

    if (m_visualizations[subViewIndex])
        return{ m_visualizations[subViewIndex].get() };

    return{};
}

void ResidualVerificationView::axesEnabledChangedEvent(bool enabled)
{
    m_implementation->setAxesVisibility(enabled);
}

void ResidualVerificationView::initialize()
{
    if (m_implementation)
        return;

    m_implementation = new RendererImplementationBase3D(*this);
    m_implementation->activate(m_qvtkMain);

    //m_strategy = new RenderViewStrategyImage2D(*m_implementation, m_implementation);
    m_strategy = new RenderViewStrategy3D(*m_implementation, m_implementation);
    m_implementation->setStrategy(m_strategy);

    m_dataSets.resize(3);
    m_visualizations.resize(3);

    for (unsigned i = 0; i < numberOfSubViews(); ++i)
    {
        auto renderer = m_implementation->renderer(i);
        renderer->SetViewport(  // left to right placement
            double(i) / double(numberOfSubViews()), 0,
            double(i + 1) / double(numberOfSubViews()), 1);
    }
}

void ResidualVerificationView::setDataInternal(unsigned int subViewIndex, DataObject * dataObject, std::vector<std::unique_ptr<AbstractVisualizedData>> & toDelete)
{
    initialize();

    m_dataSets[subViewIndex] = dataObject;

    auto & oldVis = m_visualizations[subViewIndex];

    if (oldVis)
    {
        m_implementation->removeContent(oldVis.get(), subViewIndex);

        beforeDeleteVisualization(oldVis.get());
        toDelete.push_back(std::move(oldVis));
    }

    if (dataObject)
    {
        auto newVis = m_implementation->requestVisualization(*dataObject);
        assert(newVis);
        m_visualizations[subViewIndex] = std::move(newVis);
        m_implementation->addContent(newVis.get(), subViewIndex);
    }
}

void ResidualVerificationView::updateResidual(std::vector<std::unique_ptr<AbstractVisualizedData>> & toDelete)
{
    DataObject * observation = m_dataSets[0];
    DataObject * model = m_dataSets[1];
    DataObject * residual = m_dataSets[2];

    if (!observation || !model)
    {
        if (residual)
            setDataInternal(2, nullptr, toDelete);

        return;
    }

    assert(dynamic_cast<ImageDataObject *>((observation)));

    vtkSmartPointer<vtkDataArray> observationData;
    vtkSmartPointer<vtkDataArray> modelData;
    vtkSmartPointer<vtkFloatArray> residualData;
    
    // fetch relevant data - ImageData vs. PolyData - PointData vs. CellData
    if (m_interpolateModelOnObservation)
    {
        observationData = static_cast<ImageDataObject *>(observation)->imageData()->GetPointData()->GetScalars();

        auto modelImage = dynamic_cast<ImageDataObject *>(model);
        assert(modelImage);
        modelData = modelImage->imageData()->GetPointData()->GetScalars();

        if (observationData->GetNumberOfTuples() != modelData->GetNumberOfTuples())
        {
            qDebug() << "Observation/model sizes differ, aborting";
            return;
        }

        if (!dynamic_cast<ImageDataObject *>(residual))
        {
            auto imageData = vtkSmartPointer<vtkImageData>::New();
            imageData->CopyStructure(observation->dataSet());
            imageData->AllocateScalars(VTK_FLOAT, 1);
            residualData = vtkFloatArray::SafeDownCast(imageData->GetPointData()->GetScalars());

            auto residualDataObject = std::make_unique<ImageDataObject>("Residual Image", *imageData);
            residual = residualDataObject.get();

            DataSetHandler::instance().takeData(std::move(residualDataObject));
        }
    }
    else
    {
        auto observationImageData = static_cast<ImageDataObject *>(observation)->imageData();

        auto modelPoly = dynamic_cast<PolyDataObject *>(model);
        assert(modelPoly);

        std::string observationScalars = observationImageData->GetPointData()->GetScalars()->GetName();
        auto observationPoly = interpolateImageOnPoly(*modelPoly->polyDataSet(), *observationImageData);

        observationPoly->GetCellData()->SetActiveScalars(observationScalars.c_str());
        observationData = observationPoly->GetCellData()->GetScalars();
        modelData = modelPoly->polyDataSet()->GetCellData()->GetScalars();  // simulated vectors should be projected to InSAR LoS up to here
        assert(modelData);

        if (!dynamic_cast<PolyDataObject *>(residual))
        {
            auto polyData = vtkSmartPointer<vtkPolyData>::New();
            polyData->CopyStructure(observationPoly);
            
            residualData = vtkSmartPointer<vtkFloatArray>::New();
            residualData->SetNumberOfComponents(1);
            residualData->SetNumberOfTuples(polyData->GetNumberOfCells());
            polyData->GetCellData()->SetScalars(residualData);

            auto residualDataObject = std::make_unique<PolyDataObject>("Residual PolyData", *polyData);
            residual = residualDataObject.get();

            DataSetHandler::instance().takeData(std::move(residualDataObject));
        }
    }


    assert(observationData && modelData && residualData);

    vtkIdType length = observationData->GetNumberOfTuples();
    assert(modelData->GetNumberOfTuples() == length);
    assert(residualData->GetNumberOfTuples() == length);
    
    // TODO: more flexible color mapping: use same setup for scalars with different names
    // TODO: use separate mapping for the residual view
    modelData->SetName(observationData->GetName());
    residualData->SetName(observationData->GetName());

    // TODO: parallel_for create artifacts (related to NaN values, FPU status in the threads? (see _statusfp(), _controlfp())
    threadingzeug::sequential_for(0, length, [observationData, modelData, residualData] (int i) {
        auto d = observationData->GetTuple(i)[0] - (modelData->GetTuple(i)[0]);
        residualData->SetValue(i, static_cast<float>(d));
    });

    setDataInternal(2, residual, toDelete);
}

void ResidualVerificationView::updateGuiAfterDataChange()
{
    emit visualizationsChanged();

    updateGuiSelection();


    QList<DataObject *> validImages;
    if (m_dataSets[0])
        validImages << m_dataSets[0];
    if (m_dataSets[1])
        validImages << m_dataSets[1];
    //m_strategy->setInputImages(validImages);

    if (!validImages.isEmpty() || m_dataSets[2])
        implementation().resetCamera(true);

    render();
}

void ResidualVerificationView::updateGuiSelection()
{
    updateTitle();

    DataObject * selection = nullptr;
    for (auto & vis : m_visualizations)
    {
        if (vis)
        {
            selection = &vis->dataObject();
            break;
        }
    }

    m_implementation->setSelectedData(selection);

    emit selectedDataChanged(this, selection);
}

void ResidualVerificationView::updateComboBoxes()
{
    m_observationCombo->blockSignals(true);
    m_modelCombo->blockSignals(true);


    QString oldObservationName = m_observationCombo->currentText();
    QString oldModelName = m_modelCombo->currentText();

    m_observationCombo->clear();
    m_modelCombo->clear();

    QList<ImageDataObject *> images;
    QList<PolyDataObject *> polyData2p5D;

    for (DataObject * data : DataSetHandler::instance().dataSets())
    {
        qulonglong ptrData = reinterpret_cast<size_t>(data);

        // ImageDataObjects can directly be used as observation/model
        if (dynamic_cast<ImageDataObject *>(data))
        {
            m_observationCombo->addItem(data->name(), ptrData);
            m_modelCombo->addItem(data->name(), ptrData);
            continue;
        }

        // allow to transform 2.5D poly data into model surface grid
        if (auto poly = dynamic_cast<PolyDataObject *>(data))
        {
            if (!poly->is2p5D())
                continue;

            m_modelCombo->addItem(data->name(), ptrData);
        }
    }

    m_observationCombo->setCurrentIndex(-1);
    for (int i = 0; i < m_observationCombo->count(); ++i)
    {
        if (m_observationCombo->itemText(i) != oldObservationName)
            continue;

        m_observationCombo->setCurrentIndex(i);
        break;
    }

    m_modelCombo->setCurrentIndex(-1);
    for (int i = 0; i < m_modelCombo->count(); ++i)
    {
        if (m_modelCombo->itemText(i) != oldModelName)
            continue;

        m_modelCombo->setCurrentIndex(i);
        break;
    }

    m_observationCombo->blockSignals(false);
    m_modelCombo->blockSignals(false);
}

void ResidualVerificationView::updateObservationFromUi(int index)
{
    auto data = reinterpret_cast<DataObject *>(m_observationCombo->itemData(index, Qt::UserRole).toULongLong());

    auto image = dynamic_cast<ImageDataObject *>(data);
    assert(image);

    setObservationData(image);
}

void ResidualVerificationView::updateModelFromUi(int index)
{
    auto data = reinterpret_cast<DataObject *>(m_modelCombo->itemData(index, Qt::UserRole).toULongLong());
    
    DataObject * modelData = nullptr;

    std::string modelVectorsName = "U-";
    std::string modelScalarsName = "Modeled InSAR";

    vtkPolyData * polyModel = vtkPolyData::SafeDownCast(data->dataSet());
    if (polyModel)
    {
        auto surfaceVectors = polyModel->GetCellData()->GetArray(modelVectorsName.c_str());
        if (surfaceVectors)
        {
            auto InSAR = surfaceVectorsToInSAR(*surfaceVectors, m_inSARLineOfSight);
            InSAR->SetName(modelScalarsName.c_str());
            polyModel->GetCellData()->SetScalars(InSAR);
        }
    }

    
    if (m_interpolateModelOnObservation)
    {
        // we need the model as Image data here, so create one if the input is not an image

        ImageDataObject * inputModelImage = dynamic_cast<ImageDataObject *>(data);
        QString modelImageName = data->name() + " (2D Grid)";

        if (!inputModelImage)
        {
            for (auto existing : DataSetHandler::instance().dataSets())
            {
                if (existing->name() == modelImageName)
                {
                    inputModelImage = dynamic_cast<ImageDataObject *>(existing);
                    if (inputModelImage)
                        break;
                }
            }
        }

        std::unique_ptr<ImageDataObject> newModelImage;

        if (!inputModelImage)
        {
            ImageDataObject * observation = dynamic_cast<ImageDataObject *>(m_dataSets[0]);

            if (m_dataSets.empty() || !observation)
            {
                setModelData(nullptr);
                return;
            }

            assert(polyModel);

            auto modelImageData = createImageFromPoly(*observation->imageData(), *polyModel);
            // make sure to use the InSAR model for further computations
            modelImageData->GetPointData()->SetActiveScalars(modelScalarsName.c_str());

            newModelImage = std::make_unique<ImageDataObject>(modelImageName, *modelImageData);
            modelData = newModelImage.get();
        }

        if (newModelImage)
            DataSetHandler::instance().takeData(std::move(newModelImage));
    }
    else
    {
        // pass trough the model poly data
        PolyDataObject * inputModelPoly = dynamic_cast<PolyDataObject *>(data);
        assert(inputModelPoly);
        modelData = inputModelPoly;
    }


    setModelData(modelData);
}
