#include "InteractorStyleImage.h"

#include <cmath>

#include <QTextStream>
#include <QStringList>

#include <vtkObjectFactory.h>
#include <vtkInformation.h>
#include <vtkInformationStringKey.h>
#include <vtkCallbackCommand.h>

#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkPointPicker.h>

#include <vtkPointData.h>
#include <vtkImageData.h>

#include <vtkPolyDataMapper.h>
#include <vtkDiskSource.h>
#include <vtkActor.h>
#include <vtkProperty.h>

#include <core/data_objects/DataObject.h>
#include <core/rendered_data/RenderedData.h>

#include <gui/rendering_interaction/Highlighter.h>


vtkStandardNewMacro(InteractorStyleImage);

InteractorStyleImage::InteractorStyleImage()
    : Superclass()
    , m_pointPicker(vtkSmartPointer<vtkPointPicker>::New())
    , m_highlighter(std::make_unique<Highlighter>())
    , m_mouseMoved(false)
{
}

void InteractorStyleImage::OnMouseMove()
{
    Superclass::OnMouseMove();

    int clickPos[2];
    GetInteractor()->GetEventPosition(clickPos);
    FindPokedRenderer(clickPos[0], clickPos[1]);

    m_pointPicker->Pick(clickPos[0], clickPos[1], 0, GetCurrentRenderer());

    sendPointInfo();

    m_mouseMoved = true;
}

InteractorStyleImage::~InteractorStyleImage() = default;

void InteractorStyleImage::OnLeftButtonDown()
{
    if (!GetCurrentRenderer())
        return;

    GrabFocus(EventCallbackCommand);

    StartPan();

    m_mouseMoved = false;
}

void InteractorStyleImage::OnLeftButtonUp()
{
    Superclass::OnLeftButtonUp();

    if (!m_mouseMoved)
        highlightPickedPoint();
    
    m_mouseMoved = false;
}

void InteractorStyleImage::OnMiddleButtonDown()
{
    int eventPos[2];
    GetInteractor()->GetEventPosition(eventPos);
    FindPokedRenderer(eventPos[0], eventPos[1]);

    if (!GetCurrentRenderer())
        return;

    StartDolly();
}

void InteractorStyleImage::OnMiddleButtonUp()
{
    switch (State)
    {
    case VTKIS_DOLLY:
        EndDolly();
        if (Interactor)
            ReleaseFocus();
        break;
    default:
        Superclass::OnMiddleButtonUp();
    }
}

void InteractorStyleImage::OnRightButtonDown()
{
    OnLeftButtonDown();
}

void InteractorStyleImage::OnRightButtonUp()
{
    OnLeftButtonUp();
}

void InteractorStyleImage::OnChar()
{
    // disable magic keys for now
}

void InteractorStyleImage::setRenderedData(const QList<RenderedData *> & renderedData)
{
    m_highlighter->clear();
    m_propToRenderedData.clear();

    for (RenderedData * r : renderedData)
    {
        vtkCollectionSimpleIterator it;
        r->viewProps()->InitTraversal(it);
        while (vtkProp * prop = r->viewProps()->GetNextProp(it))
        {
            if (prop->GetPickable())
                m_propToRenderedData.insert(prop, r);
        }
    }
}

void InteractorStyleImage::highlightPickedPoint()
{
    vtkIdType pointId = m_pointPicker->GetPointId();

    vtkProp * pickedProp = m_pointPicker->GetViewProp();
    if (!pickedProp)
    {
        highlightIndex(nullptr, -1);
        return;
    }

    RenderedData * renderedData = m_propToRenderedData.value(pickedProp);
    assert(renderedData);

    highlightIndex(&renderedData->dataObject(), pointId);
    
    emit dataPicked(renderedData);

    emit indexPicked(&renderedData->dataObject(), pointId);
}

DataObject * InteractorStyleImage::highlightedDataObject() const
{
    return m_highlighter->targetObject();
}

vtkIdType InteractorStyleImage::highlightedIndex() const
{
    return m_highlighter->lastTargetIndex();
}

void InteractorStyleImage::highlightIndex(DataObject * dataObject, vtkIdType index)
{
    assert(index < 0 || dataObject);

    m_highlighter->setRenderer(GetCurrentRenderer());
    m_highlighter->setTarget(dataObject, index);
}

void InteractorStyleImage::lookAtIndex(DataObject * /*polyData*/, vtkIdType /*index*/)
{
}

void InteractorStyleImage::sendPointInfo() const
{
    QString content;
    QTextStream stream;
    stream.setString(&content);

    stream.setRealNumberNotation(QTextStream::RealNumberNotation::ScientificNotation);
    stream.setRealNumberPrecision(17);

    do
    {
        vtkAbstractMapper3D * mapper = m_pointPicker->GetMapper();
        if (!mapper)
            break;


        std::string inputname;

        vtkInformation * inputInfo = mapper->GetInformation();
        if (inputInfo->Has(DataObject::NameKey()))
            inputname = DataObject::NameKey()->Get(inputInfo);

        double * pos = m_pointPicker->GetPickPosition();

        vtkDataObject * dataObject = mapper->GetInputDataObject(0, 0);
        vtkImageData * image = vtkImageData::SafeDownCast(dataObject);

        if (!image)
            break;
        
        vtkIdType pointId = m_pointPicker->GetPointId();
        double value = image->GetPointData()->GetScalars()->GetTuple(pointId)[0];

        stream
            << "input file: " << QString::fromStdString(inputname) << endl
            << "row: " << pos[0] << endl
            << "column: " << pos[1] << endl
            << "value: " << value << endl;
    } while (false);

    QStringList info;
    QString line;
    while ((line = stream.readLine()) != QString())
        info.push_back(line);

    emit pointInfoSent(info);
}
