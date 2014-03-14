#include "pickinginteractionstyle.h"

#include <vtkRenderWindowInteractor.h>

#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkPointPicker.h>
#include <vtkAbstractMapper3D.h>

#include <vtkInformation.h>
#include <vtkInformationStringKey.h>

#include <vtkPointPicker.h>

#include <QTextStream>
#include <QStringList>


#include "core/input.h"


vtkStandardNewMacro(PickingInteractionStyle);

#define VTK_CREATE(type, name) \
    vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

PickingInteractionStyle::PickingInteractionStyle()
: vtkInteractorStyleTrackballCamera()
, m_picker(vtkPointPicker::New())
{
}

void PickingInteractionStyle::OnMouseMove()
{
    pick();
    vtkInteractorStyleTrackballCamera::OnMouseMove();
}

void PickingInteractionStyle::OnLeftButtonDown()
{
    pick();
    vtkInteractorStyleTrackballCamera::OnLeftButtonDown();

    vtkIdType index = m_picker->GetPointId();
    if (index != -1)
        emit pointClicked(index);
}

void PickingInteractionStyle::pick()
{
    int* clickPos = this->GetInteractor()->GetEventPosition();

    // picking in the input geometry
    m_picker->Pick(clickPos[0], clickPos[1], 0, this->GetDefaultRenderer());

    sendPointInfo();
}

void PickingInteractionStyle::sendPointInfo() const
{
    double* pos = m_picker->GetPickPosition();

    QString content;
    QTextStream stream;
    stream.setString(&content);

    stream.setRealNumberNotation(QTextStream::RealNumberNotation::ScientificNotation);
    stream.setRealNumberPrecision(17);

    std::string inputname;

    vtkAbstractMapper3D * mapper = m_picker->GetMapper();

    if (!mapper) {
        emit pointInfoSent(QStringList());
        return;
    }

    vtkInformation * inputInfo = mapper->GetInformation();

    if (inputInfo->Has(Input::NameKey()))
        inputname = Input::NameKey()->Get(inputInfo);

    stream
        << "input file: " << QString::fromStdString(inputname) << endl
        << "selected point: " << endl
        << pos[0] << endl
        << pos[1] << endl
        << pos[2] << endl
        << "id: " << m_picker->GetPointId();

    QStringList info;
    QString line;
    while ((line = stream.readLine()) != QString())
        info.push_back(line);

    emit pointInfoSent(info);
}

