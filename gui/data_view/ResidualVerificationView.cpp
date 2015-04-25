#include <gui/data_view/ResidualVerificationView.h>

#include <cassert>
#include <functional>
#include <random>

#include <QBoxLayout>
#include <QDebug>
#include <QComboBox>
#include <QToolBar>

#include <QVTKWidget.h>
#include <vtkFloatArray.h>
#include <vtkImageData.h>
#include <vtkPointData.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>

#include <vtkAssignAttribute.h>
#include <vtkElevationFilter.h>
#include <vtkTransform.h>
#include <vtkTransformFilter.h>
#include <vtkProbeFilter.h>
#include <vtkPolyData.h>

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

vtkSmartPointer<vtkImageData> createImageFromPoly(vtkImageData * referenceGrid, vtkPolyData * poly)
{
    VTK_CREATE(vtkElevationFilter, polyElevationScalars);
    polyElevationScalars->SetInputData(poly);

    VTK_CREATE(vtkTransform, polyFlattenerTransform);
    polyFlattenerTransform->Scale(1, 1, 0);
    VTK_CREATE(vtkTransformFilter, polyFlattener);
    polyFlattener->SetTransform(polyFlattenerTransform);
    polyFlattener->SetInputConnection(polyElevationScalars->GetOutputPort());

    VTK_CREATE(vtkImageData, newGridStructure);
    newGridStructure->CopyStructure(referenceGrid);

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
    , m_disableGuiUpdate(false)
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
        if (vis && vis->dataObject() == data)
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
    // we have to call addDataObjects here for consistency with the superclass API
    const int viewIndex = 0;
    if (m_images.size() > viewIndex && m_images[viewIndex] == observation)
        return;
    if (!m_images.isEmpty() && m_images[viewIndex])
        removeDataObjects({ m_images[viewIndex] });
    if (observation)
    {
        QList<DataObject *> incompatible;
        addDataObjects({ observation }, incompatible, viewIndex);
        assert(incompatible.isEmpty()); // the function interface already enforces compatibility
    }
}

void ResidualVerificationView::setModelData(ImageDataObject * model)
{
    const int viewIndex = 1;
    if (m_images.size() > viewIndex && m_images[viewIndex] == model)
        return;
    if (!m_images.isEmpty() && m_images[viewIndex])
        hideDataObjects({ m_images[viewIndex] });
    if (model)
    {
        QList<DataObject *> incompatible;
        addDataObjects({ model }, incompatible, viewIndex);
        assert(incompatible.isEmpty());
    }
}

void ResidualVerificationView::setResidualData(ImageDataObject * residual)
{
    const int viewIndex = 2;
    if (m_images.size() > viewIndex && m_images[viewIndex] == residual)
        return;
    if (!m_images.isEmpty() && m_images[viewIndex])
        removeDataObjects({ m_images[viewIndex] });
    if (residual)
    {
        QList<DataObject *> incompatible;
        addDataObjects({ residual }, incompatible, viewIndex);
        assert(incompatible.isEmpty());
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

void ResidualVerificationView::addDataObjectsImpl(const QList<DataObject *> & dataObjects,
    QList<DataObject *> & incompatibleObjects,
    unsigned int subViewIndex)
{
    if (dataObjects.size() > 1)
        qDebug() << "Multiple objects per sub-view not supported in the ResidualVerificationView.";

    for (int i = 1; i < dataObjects.size(); ++i)
        incompatibleObjects << dataObjects[i];

    auto data = dataObjects.isEmpty() ? nullptr : dataObjects.first();
    auto imageData = dynamic_cast<ImageDataObject *>(data);
    if (!imageData)
    {
        qDebug() << "ResidualVerificationView only supports ImageDataObjects!";
        incompatibleObjects.prepend(data);
    }

    setData(subViewIndex, imageData);

    emit visualizationsChanged();

    updateGuiSelection();

    if (imageData)
        implementation().resetCamera(true);

    render();
}

void ResidualVerificationView::hideDataObjectsImpl(const QList<DataObject *> & dataObjects, unsigned int subViewIndex)
{
    assert(m_visualizations.size() > int(subViewIndex));

    auto && vis = m_visualizations[subViewIndex];

    if (!vis)
        return;

    for (auto && data : dataObjects)
    {
        if (vis->dataObject() != data)
            continue;

        // don't cache for now
        emit beforeDeleteVisualization(vis);
        delete vis;
        vis = nullptr;
    }

    updateGuiSelection();

    emit visualizationsChanged();

    render();
}

void ResidualVerificationView::removeDataObjectsImpl(const QList<DataObject *> & dataObjects)
{
    for (auto toDelete : dataObjects)
    {
        for (int i = 0; i < m_images.size(); ++i)
        {
            if (toDelete == m_images[i])
                setData(i, nullptr);
        }
    }

    emit visualizationsChanged();

    updateGuiSelection();

    render();
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

void ResidualVerificationView::setData(unsigned int subViewIndex, ImageDataObject * dataObject)
{
    initialize();

    if (m_images[subViewIndex] == dataObject)
    {
        if (dataObject) // make sure that we are really showing this data object
            m_implementation->addContent(m_visualizations[subViewIndex]);
        return;
    }

    m_images[subViewIndex] = dataObject;

    auto oldVis = m_visualizations[subViewIndex];

    if (oldVis)
    {
        m_implementation->removeContent(oldVis);

        beforeDeleteVisualization(oldVis);
        delete oldVis;
    }

    if (dataObject)
    {
        auto newVis = m_implementation->requestVisualization(dataObject);
        assert(newVis);
        m_visualizations[subViewIndex] = newVis;
        m_implementation->addContent(newVis, subViewIndex);
    }

    QList<ImageDataObject *> validImages;
    if (m_images[0])
        validImages << m_images[0];
    if (m_images[1])
        validImages << m_images[1];
    m_strategy->setInputImages(validImages);

    if (subViewIndex != 2)
        updateResidual();
}

void ResidualVerificationView::updateResidual()
{
    ImageDataObject *  observation = m_images[0];
    ImageDataObject *  model = m_images[1];
    ImageDataObject * residual = m_images[2];

    if (!observation || !model)
    {
        if (residual)
            hideDataObjects({ residual }, 2);
        return;
    }

    auto observationData = observation->imageData()->GetPointData()->GetScalars();
    auto modelData = model->imageData()->GetPointData()->GetScalars();
    
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

        residual = new ImageDataObject("Residual", imageData);

        DataSetHandler::instance().addData({ residual });
    }

    vtkIdType length = observationData->GetNumberOfTuples();

    auto residualData = vtkFloatArray::SafeDownCast(residual->imageData()->GetPointData()->GetScalars());
    assert(residualData);

    for (vtkIdType i = 0; i < length; ++i)
    {
        float o = (float)observationData->GetTuple(i)[0];
        float m = (float)modelData->GetTuple(i)[0];
        float d = o - m;
        residualData->SetValue(i, d);
    }

    setData(2, residual);
}

void ResidualVerificationView::updateGuiSelection()
{
    updateTitle();

    DataObject * selection = nullptr;
    for (auto vis : m_visualizations)
    {
        if (vis)
        {
            selection = vis->dataObject();
            break;
        }
    }

    m_implementation->setSelectedData(selection);

    emit selectedDataChanged(this, selection);
}

void ResidualVerificationView::updateComboBoxes()
{
    if (m_disableGuiUpdate)
        return;

    m_disableGuiUpdate = true;

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

    m_disableGuiUpdate = false;
}

void ResidualVerificationView::updateObservationFromUi(int index)
{
    if (m_disableGuiUpdate)
        return;

    m_disableGuiUpdate = true;

    auto data = reinterpret_cast<DataObject *>(m_observationCombo->itemData(index, Qt::UserRole).toULongLong());

    auto image = dynamic_cast<ImageDataObject *>(data);
    assert(image);

    setObservationData(image);

    m_disableGuiUpdate = false;
}

void ResidualVerificationView::updateModelFromUi(int index)
{
    if (m_disableGuiUpdate)
        return;

    m_disableGuiUpdate = true;

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

    bool newModel = false;

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

        auto modelImageData = createImageFromPoly(observation->imageData(), polyModel);
        if (auto uplus = modelImageData->GetPointData()->GetArray("U+"))
        {
            modelImageData->GetPointData()->SetScalars(uplus);
        }

        image = new ImageDataObject(modelImageName, modelImageData);
        newModel = true;
    }

    setModelData(image);
    if (newModel)
        DataSetHandler::instance().addData({ image });

    m_disableGuiUpdate = false;
}
