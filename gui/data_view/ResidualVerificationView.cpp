#include <gui/data_view/ResidualVerificationView.h>

#include <cassert>
#include <functional>
#include <random>

#include <QBoxLayout>
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
#include <vtkPolyData.h>
#include <vtkProbeFilter.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkTransform.h>
#include <vtkTransformFilter.h>

#include <threadingzeug/parallelfor.h>

#include <core/AbstractVisualizedData.h>
#include <core/DataSetHandler.h>
#include <core/types.h>
#include <core/color_mapping/ColorMapping.h>
#include <core/data_objects/ImageDataObject.h>
#include <core/data_objects/PolyDataObject.h>
#include <core/utility/vtkhelper.h>
#include <gui/SelectionHandler.h>
#include <gui/data_view/RendererImplementationBase3D.h>
#include <gui/data_view/RenderViewStrategyImage2D.h>


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
    VTK_CREATE(vtkElevationFilter, polyElevationScalars);
    polyElevationScalars->SetInputData(&poly);

    VTK_CREATE(vtkTransform, polyFlattenerTransform);
    polyFlattenerTransform->Scale(1, 1, 0);
    VTK_CREATE(vtkTransformFilter, polyFlattener);
    polyFlattener->SetTransform(polyFlattenerTransform);
    polyFlattener->SetInputConnection(polyElevationScalars->GetOutputPort());

    VTK_CREATE(vtkImageData, newGridStructure);
    newGridStructure->CopyStructure(&referenceGrid);

    VTK_CREATE(vtkProbeFilter, probe);
    probe->SetInputData(newGridStructure);
    probe->SetSourceConnection(polyFlattener->GetOutputPort());

    probe->Update();
    vtkSmartPointer<vtkImageData> newImage = vtkImageData::SafeDownCast(probe->GetOutput());

    return newImage;
}

}


ResidualVerificationView::ResidualVerificationView(int index, QWidget * parent, Qt::WindowFlags flags)
    : AbstractRenderView(index, parent, flags)
    , m_qvtkMain(nullptr)
    , m_observationCombo(nullptr)
    , m_modelCombo(nullptr)
    , m_inSARLineOfSight(0, 0, 1)
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

    for (auto vis : m_visualizations)
    {
        if (!vis)
            continue;

        beforeDeleteVisualization(vis);
        delete vis;
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
            return vis;
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
        for (int i = 0; i < m_images.size(); ++i)
        {
            if (m_images[i] == dataObject && m_visualizations.size() > i)
                return m_visualizations[i];
        }
        return nullptr;
    }

    assert(subViewIndex >= 0);
    assert(m_images.size() >= m_visualizations.size());
    if (m_images[subViewIndex] != dataObject || m_visualizations.size() < subViewIndex)
        return nullptr;

    return m_visualizations[subViewIndex];
}

void ResidualVerificationView::setObservationData(ImageDataObject * observation)
{
    setDataHelper(0, observation);
}

void ResidualVerificationView::setModelData(ImageDataObject * model)
{
    setDataHelper(1, model);
}

void ResidualVerificationView::setResidualData(ImageDataObject * residual)
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

void ResidualVerificationView::setDataHelper(unsigned int subViewIndex, ImageDataObject * data, bool skipGuiUpdate, QList<AbstractVisualizedData *> * toDelete)
{
    assert(skipGuiUpdate == (toDelete != nullptr));

    if (m_images.size() > int(subViewIndex) && m_images[subViewIndex] == data)
        return;

    QList<AbstractVisualizedData *> toDeleteInternal;
    setDataInternal(subViewIndex, data, toDeleteInternal);

    // create a residual only if we didn't just set one
    if (subViewIndex != 2 || !data)
        updateResidual(toDeleteInternal);

    // update GUI before actually deleting old visualization data

    if (skipGuiUpdate)
    {
        toDelete->append(toDeleteInternal);
    }
    else
    {
        updateGuiAfterDataChange();
        qDeleteAll(toDeleteInternal);
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

    assert(m_images.size() < int(subViewIndex) || m_images[subViewIndex] == nullptr || m_images[subViewIndex] == imageData);

    setDataHelper(subViewIndex, imageData);
}

void ResidualVerificationView::hideDataObjectsImpl(const QList<DataObject *> & dataObjects, unsigned int subViewIndex)
{
    assert(unsigned(m_images.size()) > subViewIndex);

    // no caching for now, just remove the object
    if (dataObjects.contains(m_images[subViewIndex]))
        setDataHelper(subViewIndex, nullptr);
}

QList<DataObject *> ResidualVerificationView::dataObjectsImpl(int subViewIndex) const
{
    if (subViewIndex == -1)
    {
        QList<DataObject *> objects;

        for (auto && image : m_images)
            if (image)
                objects << image;

        return objects;
    }

    if (m_images.size() > subViewIndex && m_images[subViewIndex])
        return{ m_images[subViewIndex] };
        
    return{};
}

void ResidualVerificationView::prepareDeleteDataImpl(const QList<DataObject *> & dataObjects)
{
    QList<AbstractVisualizedData *> toDelete;

    for (auto objectToDelete : dataObjects)
    {
        for (int i = 0; i < m_images.size(); ++i)
        {
            if (objectToDelete == m_images[i])
                setDataHelper(i, nullptr, true, &toDelete);
        }
    }

    updateGuiAfterDataChange();

    qDeleteAll(toDelete);
}

QList<AbstractVisualizedData *> ResidualVerificationView::visualizationsImpl(int subViewIndex) const
{
    QList<AbstractVisualizedData *> validVis;

    if (subViewIndex == -1)
    {
        for (auto && vis : m_visualizations)
            if (vis)
                validVis << vis;
        return validVis;
    }

    if (m_visualizations[subViewIndex])
        return{ m_visualizations[subViewIndex] };

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

    m_strategy = new RenderViewStrategyImage2D(*m_implementation, m_implementation);
    m_implementation->setStrategy(m_strategy);

    m_images.resize(3);
    m_visualizations.resize(3);

    for (unsigned i = 0; i < numberOfSubViews(); ++i)
    {
        auto renderer = m_implementation->renderer(i);
        renderer->SetViewport(  // left to right placement
            double(i) / double(numberOfSubViews()), 0,
            double(i + 1) / double(numberOfSubViews()), 1);
    }
}

void ResidualVerificationView::setDataInternal(unsigned int subViewIndex, ImageDataObject * dataObject, QList<AbstractVisualizedData *> & toDelete)
{
    initialize();

    m_images[subViewIndex] = dataObject;

    auto && oldVis = m_visualizations[subViewIndex];

    if (oldVis)
    {
        m_implementation->removeContent(oldVis, subViewIndex);

        beforeDeleteVisualization(oldVis);
        toDelete << oldVis;
        oldVis = nullptr;
    }

    if (dataObject)
    {
        auto newVis = m_implementation->requestVisualization(dataObject);
        assert(newVis);
        m_visualizations[subViewIndex] = newVis;
        m_implementation->addContent(newVis, subViewIndex);
    }
}

void ResidualVerificationView::updateResidual(QList<AbstractVisualizedData *> & toDelete)
{
    ImageDataObject * observation = m_images[0];
    ImageDataObject * model = m_images[1];
    ImageDataObject * residual = m_images[2];

    if (!observation || !model)
    {
        if (residual)
            setDataInternal(2, nullptr, toDelete);

        return;
    }

    auto observationData = observation->imageData()->GetPointData()->GetScalars();
    auto modelData = model->imageData()->GetPointData()->GetScalars();
    assert(vtkFloatArray::SafeDownCast(observationData) && vtkFloatArray::SafeDownCast(modelData)); // TODO generalize the hacks below
    
    if (observationData->GetNumberOfTuples() != modelData->GetNumberOfTuples())
    {
        qDebug() << "Observation/model sizes differ, aborting";
        return;
    }

    if (!residual)
    {
        VTK_CREATE(vtkImageData, imageData);
        imageData->CopyStructure(observation->imageData());
        imageData->AllocateScalars(VTK_FLOAT, 1);

        auto residualData = std::make_unique<ImageDataObject>("Residual", *imageData);
        residual = residualData.get();

        DataSetHandler::instance().takeData(std::move(residualData));
    }

    vtkIdType length = observationData->GetNumberOfTuples();

    auto residualData = vtkFloatArray::SafeDownCast(residual->imageData()->GetPointData()->GetScalars());
    assert(residualData);
    residualData->SetName("Residual");

    // parallel_for create artifacts (related to NaN values, FPU status in the threads? (see _statusfp(), _controlfp())
    threadingzeug::sequential_for(0, length, [observationData, modelData, residualData] (int i) {
        auto d = observationData->GetTuple(i)[0] - (modelData->GetTuple(i)[0]);
        residualData->SetValue(i, d);
    });

    setDataInternal(2, residual, toDelete);
}

void ResidualVerificationView::updateGuiAfterDataChange()
{
    emit visualizationsChanged();

    updateGuiSelection();


    QList<ImageDataObject *> validImages;
    if (m_images[0])
        validImages << m_images[0];
    if (m_images[1])
        validImages << m_images[1];
    m_strategy->setInputImages(validImages);

    if (!validImages.isEmpty() || m_images[2])
        implementation().resetCamera(true);

    render();
}

void ResidualVerificationView::updateGuiSelection()
{
    updateTitle();

    DataObject * selection = nullptr;
    for (auto vis : m_visualizations)
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
    
    ImageDataObject * image = dynamic_cast<ImageDataObject *>(data);

    QString modelImageName = data->name() + " (2D Grid)";

    if (!image)
    {
        for (auto existing : DataSetHandler::instance().dataSets())
        {
            if (existing->name() == modelImageName)
            {
                image = dynamic_cast<ImageDataObject *>(existing);
                if (image)
                    break;
            }
        }
    }

    std::unique_ptr<ImageDataObject> newModelImage;

    if (!image)
    {
        if (m_images.isEmpty() || !m_images[0])
        {
            setModelData(nullptr);
            return;
        }

        ImageDataObject * observation = m_images[0];

        vtkPolyData * polyModel = vtkPolyData::SafeDownCast(data->dataSet());
        assert(polyModel);

        std::string modelScalarsName = "Modeled InSAR";
        auto surfaceVectors = polyModel->GetCellData()->GetArray("U-");
        if (surfaceVectors)
        {
            auto InSAR = surfaceVectorsToInSAR(*surfaceVectors, m_inSARLineOfSight);
            InSAR->SetName(modelScalarsName.c_str());
            polyModel->GetCellData()->SetScalars(InSAR);
        }

        auto modelImageData = createImageFromPoly(*observation->imageData(), *polyModel);
        // make sure to use the InSAR model for further computations
        modelImageData->GetPointData()->SetActiveScalars(modelScalarsName.c_str());

        newModelImage = std::make_unique<ImageDataObject>(modelImageName, *modelImageData);
        image = newModelImage.get();
    }

    setModelData(image);
    if (newModelImage)
        DataSetHandler::instance().takeData(std::move(newModelImage));

}
