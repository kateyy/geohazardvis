#include <gui/data_view/ResidualVerificationView.h>

#include <cassert>
#include <functional>
#include <random>

#include <QBoxLayout>

#include <vtkFloatArray.h>
#include <vtkImageData.h>
#include <vtkPointData.h>

#include <core/DataSetHandler.h>
#include <core/vtkhelper.h>
#include <core/data_objects/ImageDataObject.h>
#include <gui/data_view/RenderView.h>


ResidualVerificationView::ResidualVerificationView(int index, QWidget * parent, Qt::WindowFlags flags)
    : AbstractDataView(index, parent, flags)
    , m_observation(nullptr)
    , m_model(nullptr)
    , m_residual(nullptr)
{
    setLayout(new QBoxLayout(QBoxLayout::Direction::LeftToRight));
}

bool ResidualVerificationView::isTable() const
{
    return false;
}

bool ResidualVerificationView::isRenderer() const
{
    return false;
}

QString ResidualVerificationView::friendlyName() const
{
    return "Observation, Model, Residual";
}

void ResidualVerificationView::setObservationData(ImageDataObject * observation)
{
    initialize();

    m_observation = observation;

    QList<DataObject *> incompatible;
    m_renderViews[0]->addDataObjects({ observation }, incompatible);
    assert(incompatible.isEmpty());


    auto image = observation->imageData();



    VTK_CREATE(vtkImageData, model);
    model->DeepCopy(image);
    auto data = model->GetPointData()->GetScalars();
    auto floatData = vtkFloatArray::SafeDownCast(data);
    if (!floatData)
        return;

    double range[2];
    floatData->GetRange(range);
    float r = float(range[1] - range[0]) * 0.1;

    std::mt19937 engine;
    std::uniform_real_distribution<float> dist(-r, r);
    auto rnd = std::bind(dist, engine);

    vtkIdType numValues = floatData->GetNumberOfTuples() * floatData->GetNumberOfComponents();
    float * ptr = floatData->GetPointer(0);
    for (vtkIdType i = 0; i < numValues; ++i)
    {
        ptr[i] += rnd();
    }

    ImageDataObject * modelObject = new ImageDataObject("Fake Model", model);

    setModelData(modelObject);

    //updateResidual();
}

void ResidualVerificationView::setModelData(ImageDataObject * model)
{
    initialize();

    m_model = model;

    QList<DataObject *> incompatible;
    m_renderViews[1]->addDataObjects({ model }, incompatible);
    assert(incompatible.isEmpty());

    updateResidual();
}

void ResidualVerificationView::showEvent(QShowEvent * event)
{
    AbstractDataView::showEvent(event);

    initialize();
}

QWidget * ResidualVerificationView::contentWidget()
{
    return this;
}

void ResidualVerificationView::highlightedIdChangedEvent(DataObject * dataObject, vtkIdType itemId)
{
    for (auto view : m_renderViews)
    {
        if (view->dataObjects().contains(dataObject))
            view->setHighlightedId(dataObject, itemId);
    }
}

void ResidualVerificationView::initialize()
{
    if (!m_renderViews.isEmpty())
        return;

    for (int i = 0; i < 3; ++i)
    {
        auto view = new RenderView(-1);
        m_renderViews << view;
        layout()->addWidget(view);
    }
}

void ResidualVerificationView::updateResidual()
{
    if (!m_observation || !m_model)
        return;

    auto obj = m_observation->imageData()->GetPointData()->GetScalars();
    auto mdl = m_model->imageData()->GetPointData()->GetScalars();

    vtkIdType length = std::min(obj->GetNumberOfTuples(), mdl->GetNumberOfTuples());

    VTK_CREATE(vtkFloatArray, res);
    res->SetNumberOfValues(length);

    for (int i = 0; i < length; ++i)
    {
        float value = float(obj->GetTuple(i)[0] - mdl->GetTuple(i)[0]);
        res->SetValue(i, value);
    }

    VTK_CREATE(vtkImageData, image);
    image->GetPointData()->SetScalars(res);

    image->SetDimensions(m_observation->imageData()->GetDimensions());
    image->SetExtent(m_observation->imageData()->GetExtent());
    image->SetSpacing(m_observation->imageData()->GetSpacing());
    image->SetOrigin(m_observation->imageData()->GetOrigin());

    ImageDataObject * residual = new ImageDataObject("Residual", image);
    DataSetHandler::instance().addData({ residual });

    QList<DataObject *> incompatible;
    m_renderViews[2]->addDataObjects({ residual }, incompatible);
}

