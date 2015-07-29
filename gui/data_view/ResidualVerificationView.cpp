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
#include <vtkCellData.h>
#include <vtkImageData.h>
#include <vtkMath.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>

#include <threadingzeug/parallelfor.h>

#include <core/AbstractVisualizedData.h>
#include <core/DataSetHandler.h>
#include <core/types.h>
#include <core/color_mapping/ColorMapping.h>
#include <core/data_objects/ImageDataObject.h>
#include <core/data_objects/PolyDataObject.h>
#include <core/utility/DataExtent.h>
#include <core/utility/InterpolationHelper.h>

#include <gui/SelectionHandler.h>
#include <gui/data_view/RendererImplementationResidual.h>
#include <gui/data_view/RenderViewStrategyImage2D.h>
#include <gui/data_view/RenderViewStrategy3D.h>


namespace
{

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

}


ResidualVerificationView::ResidualVerificationView(int index, QWidget * parent, Qt::WindowFlags flags)
    : AbstractRenderView(index, parent, flags)
    , m_qvtkMain(nullptr)
    , m_observationCombo(nullptr)
    , m_modelCombo(nullptr)
    , m_inSARLineOfSight(0, 0, 1)
    , m_interpolationMode(InterpolationMode::observationToModel)
    , m_observationUnitFactor(1.0)
    , m_modelUnitFactor(1.0)
    , m_observationData(nullptr)
    , m_modelData(nullptr)
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
    interpolationSwitch->setChecked(m_interpolationMode == InterpolationMode::observationToModel);
    // TODO correctly link in both ways
    connect(interpolationSwitch, &QAbstractButton::toggled, [this](bool checked)
    {
        setInterpolationMode(checked
            ? InterpolationMode::observationToModel
            : InterpolationMode::modelToObservation);
    });
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
        vis.reset();
    }

    if (m_implementation)
    {
        m_implementation->deactivate(m_qvtkMain);
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
        for (unsigned int i = 0; i < numberOfSubViews(); ++i)
        {
            if (dataAt(i) == dataObject)
                return m_visualizations[i].get();
        }
        return nullptr;
    }

    assert(subViewIndex >= 0 && subViewIndex < 3);

    bool validAccess = dataAt(unsigned(subViewIndex)) == dataObject;
    if (!validAccess)
        return nullptr;

    return m_visualizations[subViewIndex].get();
}

void ResidualVerificationView::setObservationData(DataObject * observation)
{
    setDataHelper(observationIndex, observation);
}

void ResidualVerificationView::setModelData(DataObject * model)
{
    setDataHelper(modelIndex, model);
}

void ResidualVerificationView::setResidualData(DataObject * residual)
{
    setDataHelper(residualIndex, residual);
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

void ResidualVerificationView::setInterpolationMode(InterpolationMode mode)
{
    if (m_interpolationMode == mode)
        return;

    m_interpolationMode = mode;

    //updateResidual();
}

ResidualVerificationView::InterpolationMode ResidualVerificationView::interpolationMode() const
{
    return m_interpolationMode;
}

void ResidualVerificationView::setDataHelper(unsigned int subViewIndex, DataObject * data, bool skipGuiUpdate, std::vector<std::unique_ptr<AbstractVisualizedData>> * toDelete)
{
    assert(skipGuiUpdate == (toDelete != nullptr));

    if (dataAt(subViewIndex) == data)
        return;

    std::vector<std::unique_ptr<AbstractVisualizedData>> toDeleteInternal;
    setDataInternal(subViewIndex, data, toDeleteInternal);

    std::unique_ptr<DataObject> oldResidual; // to be deleted at the end of this function

    // create a residual only if we didn't just set one
    if (subViewIndex != 2 || !data)
        oldResidual = updateResidual(toDeleteInternal);

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
    return numberOfViews;
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

    setDataHelper(subViewIndex, data);
}

void ResidualVerificationView::hideDataObjectsImpl(const QList<DataObject *> & dataObjects, unsigned int subViewIndex)
{
    // no caching for now, just remove the object
    bool relevantRequest = dataObjects.contains(dataAt(subViewIndex));

    if (relevantRequest)
        setDataHelper(subViewIndex, nullptr);
}

QList<DataObject *> ResidualVerificationView::dataObjectsImpl(int subViewIndex) const
{
    if (subViewIndex == -1)
    {
        QList<DataObject *> objects;
        for (unsigned i = 0; i < numberOfSubViews(); ++i)
        {
            if (auto data = dataAt(i))
                objects << data;
        }

        return objects;
    }

    if (auto data = dataAt(unsigned(subViewIndex)))
        return{ data };

    return{};
}

void ResidualVerificationView::prepareDeleteDataImpl(const QList<DataObject *> & dataObjects)
{
    std::vector<std::unique_ptr<AbstractVisualizedData>> toDelete;

    if (dataObjects.contains(m_observationData))
        setDataHelper(observationIndex, nullptr, true, &toDelete);
    if (dataObjects.contains(m_modelData))
        setDataHelper(modelIndex, nullptr, true, &toDelete);
    assert(!dataObjects.contains(m_residual.get()));

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

    m_implementation = std::make_unique<RendererImplementationResidual>(*this);
    m_implementation->activate(m_qvtkMain);

    m_strategy = new RenderViewStrategyImage2D(*m_implementation, m_implementation.get());

    m_implementation->setStrategy(m_strategy);

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

    if (subViewIndex != residualIndex)
    {
        setDataAt(subViewIndex, dataObject);
    }
    else
    {
        // std::move for the residual cannot be done here
        assert(dataAt(subViewIndex) == dataObject);
    }

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
        auto newVisPtr = newVis.get();
        m_visualizations[subViewIndex] = std::move(newVis);
        m_implementation->addContent(newVisPtr, subViewIndex);
    }
}

std::unique_ptr<DataObject> ResidualVerificationView::updateResidual(std::vector<std::unique_ptr<AbstractVisualizedData>> & toDelete)
{
    if (!m_observationData || !m_modelData)
    {
        if (!m_residual)
            return{};

        auto oldResidual = std::move(m_residual);
        setDataInternal(residualIndex, nullptr, toDelete);
        return oldResidual;
    }

    QString observationAttributeName;
    bool useObservationCellData = false;
    QString modelAttributeName;
    bool useModelCellData = false;

    assert(m_observationData->dataSet());
    auto & observationDataSet = *m_observationData->dataSet();
    assert(m_modelData->dataSet());
    auto & modelDataSet = *m_modelData->dataSet();


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

    if (auto poly = vtkPolyData::SafeDownCast(m_modelData->dataSet()))
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
    {
        qDebug() << "Cannot find suitable data attributes";
        if (!m_residual)
            return{};

        auto oldResidual = std::move(m_residual);
        setDataInternal(residualIndex, nullptr, toDelete);
        return oldResidual;
    }


    vtkSmartPointer<vtkDataArray> observationData;
    vtkSmartPointer<vtkDataArray> modelData;

    if (m_interpolationMode == InterpolationMode::modelToObservation)
    {
        observationData = observationAttributeName.isEmpty()
            ? observationDataSet.GetPointData()->GetScalars()
            : observationDataSet.GetPointData()->GetArray(observationAttributeName.toUtf8().data());

        modelData = InterpolationHelper::interpolate(observationDataSet, modelDataSet, modelAttributeName, useModelCellData);
    }
    else
    {
        observationData = InterpolationHelper::interpolate(modelDataSet, observationDataSet, observationAttributeName, useObservationCellData);

        modelData = useModelCellData
            ? modelDataSet.GetCellData()->GetArray(modelAttributeName.toUtf8().data())
            : modelDataSet.GetPointData()->GetArray(modelAttributeName.toUtf8().data());
    }

    if (!observationData || !modelData)
    {
        qDebug() << "Observation/Model interpolation failed";
        if (!m_residual)
            return{};

        auto oldResidual = std::move(m_residual);
        setDataInternal(residualIndex, nullptr, toDelete);
        return oldResidual;
    }

    // project vectors if needed
    if (observationData->GetNumberOfComponents() == 3)
    {
        observationData = projectToLineOfSight(*observationData, m_inSARLineOfSight);
    }
    if (modelData->GetNumberOfComponents() == 3)
    {
        modelData = projectToLineOfSight(*modelData, m_inSARLineOfSight);
    }

    assert(modelData->GetNumberOfComponents() == 1);
    assert(observationData->GetNumberOfComponents() == 1);
    assert(modelData->GetNumberOfTuples() == observationData->GetNumberOfTuples());


    // compute the residual data

    auto & referenceDataSet = m_interpolationMode == InterpolationMode::modelToObservation
        ? observationDataSet
        : modelDataSet;

    auto residualData = vtkSmartPointer<vtkDataArray>::Take(modelData->NewInstance());
    residualData->SetName("Residual");
    residualData->SetNumberOfTuples(modelData->GetNumberOfTuples());
    residualData->SetNumberOfComponents(1);

    for (vtkIdType i = 0; i < residualData->GetNumberOfTuples(); ++i)
    {
        auto o_value = observationData->GetTuple(i)[0] * m_observationUnitFactor;
        auto m_value = modelData->GetTuple(i)[0] * m_modelUnitFactor;

        double r_value = o_value - m_value;
        residualData->SetTuple(i, &r_value);
    }


    std::unique_ptr<DataObject> residualReplacement;

    auto residual = vtkSmartPointer<vtkDataSet>::Take(referenceDataSet.NewInstance());
    residual->CopyStructure(&referenceDataSet);

    // TODO do not recreate data DataObject if not needed
    if (auto residualImage = vtkImageData::SafeDownCast(residual))
    {
        assert(residualData->GetNumberOfTuples() == residual->GetNumberOfPoints());
        qDebug() << residualData->GetName();

        residual->GetPointData()->SetScalars(residualData);

        residualReplacement = std::make_unique<ImageDataObject>("Residual", *residualImage);
    }
    else if (auto residualPoly = vtkPolyData::SafeDownCast(residual))
    {
        assert(vtkPolyData::SafeDownCast(residual));
        // assuming that we store attributes in polygonal data always per cell
        assert(residual->GetNumberOfCells() == residualData->GetNumberOfTuples());

        residual->GetCellData()->SetScalars(residualData);

        residualReplacement = std::make_unique<PolyDataObject>("Residual", *residualPoly);
    }
    else
    {
        qDebug() << "Residual creation failed";
        if (!m_residual)
            return{};

        auto oldResidual = std::move(m_residual);
        setDataInternal(residualIndex, nullptr, toDelete);
        return oldResidual;
    }


    // TODO improve the switch performance
    auto oldResidual = std::move(m_residual);
    setDataInternal(residualIndex, nullptr, toDelete);
    m_residual = std::move(residualReplacement);
    setDataInternal(residualIndex, m_residual.get(), toDelete);
    return oldResidual;
}

void ResidualVerificationView::updateGuiAfterDataChange()
{
    emit visualizationsChanged();

    updateGuiSelection();

    QList<DataObject *> validInputData;
    if (m_observationData)
        validInputData << m_observationData;
    if (m_modelData)
        validInputData << m_modelData;
    m_strategy->setInputData(validInputData);

    if (!validInputData.isEmpty())
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

        // allow to transform 2.5D polygonal data into model surface grid
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

    setObservationData(data);
}

void ResidualVerificationView::updateModelFromUi(int index)
{
    auto data = reinterpret_cast<DataObject *>(m_modelCombo->itemData(index, Qt::UserRole).toULongLong());

    setModelData(data);
}

DataObject * ResidualVerificationView::dataAt(unsigned int i) const
{
    switch (i)
    {
    case observationIndex:
        return m_observationData;
    case modelIndex:
        return m_modelData;
    case residualIndex:
        return m_residual.get();
    }
    assert(false);
    return nullptr;
}

bool ResidualVerificationView::setDataAt(unsigned int i, DataObject * data)
{
    switch (i)
    {
    case observationIndex:
        if (m_observationData == data)
            return false;
        m_observationData = data;
        break;
    case modelIndex:
        if (m_modelData == data)
            return false;
        m_modelData = data;
        break;
    default:
        assert(false);
        return false;
    }
    return true;
}
