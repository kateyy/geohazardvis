#include "pickinginteractionstyle.h"

#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>

#include <vtkObjectFactory.h>
#include <vtkPointPicker.h>
#include <vtkCellPicker.h>
#include <vtkAbstractMapper3D.h>

#include <vtkInformation.h>
#include <vtkInformationStringKey.h>

#include <vtkIdTypeArray.h>
#include <vtkSelectionNode.h>
#include <vtkSelection.h>
#include <vtkExtractSelection.h>
#include <vtkUnstructuredGrid.h>
#include <vtkDataSetMapper.h>
#include <vtkActor.h>
#include <vtkProperty.h>
#include <vtkCamera.h>
#include <vtkFloatArray.h>
#include <vtkTriangle.h>
#include <vtkMath.h>
#include <vtkPolyData.h>

#include <QTextStream>
#include <QStringList>
#include <QItemSelectionModel>

#include "core/vtkhelper.h"
#include "core/input.h"


vtkStandardNewMacro(PickingInteractionStyle);

PickingInteractionStyle::PickingInteractionStyle()
: vtkInteractorStyleTrackballCamera()
, m_pointPicker(vtkSmartPointer<vtkPointPicker>::New())
, m_cellPicker(vtkSmartPointer<vtkCellPicker>::New())
, m_selectedCellActor(vtkSmartPointer<vtkActor>::New())
, m_selectedCellMapper(vtkSmartPointer<vtkDataSetMapper>::New())
{
}

void PickingInteractionStyle::OnMouseMove()
{
    pickPoint();
    vtkInteractorStyleTrackballCamera::OnMouseMove();
}

void PickingInteractionStyle::OnLeftButtonDown()
{
    pickCell();

    vtkInteractorStyleTrackballCamera::OnLeftButtonDown();
}

void PickingInteractionStyle::pickPoint()
{
    int* clickPos = GetInteractor()->GetEventPosition();

    // picking in the input geometry
    m_pointPicker->Pick(clickPos[0], clickPos[1], 0, GetDefaultRenderer());

    sendPointInfo();
}

void PickingInteractionStyle::pickCell()
{
    int* clickPos = GetInteractor()->GetEventPosition();

    m_cellPicker->Pick(clickPos[0], clickPos[1], 0, GetDefaultRenderer());
    vtkIdType cellId = m_cellPicker->GetCellId();

    if (cellId != -1)
        highlightCell(cellId, m_cellPicker->GetDataSet());
}

void PickingInteractionStyle::highlightCell(int cellId, vtkDataObject * dataObject)
{
    assert(dataObject);

    VTK_CREATE(vtkIdTypeArray, ids);
    ids->SetNumberOfComponents(1);
    ids->InsertNextValue(cellId);

    VTK_CREATE(vtkSelectionNode, selectionNode);
    selectionNode->SetFieldType(vtkSelectionNode::CELL);
    selectionNode->SetContentType(vtkSelectionNode::INDICES);
    selectionNode->SetSelectionList(ids);

    VTK_CREATE(vtkSelection, selection);
    selection->AddNode(selectionNode);

    VTK_CREATE(vtkExtractSelection, extractSelection);
    extractSelection->SetInputData(0, dataObject);
    extractSelection->SetInputData(1, selection);
    extractSelection->Update();

    VTK_CREATE(vtkUnstructuredGrid, selected);
    selected->ShallowCopy(extractSelection->GetOutput());

    m_selectedCellMapper->SetInputData(selected);

    m_selectedCellActor->SetMapper(m_selectedCellMapper);
    m_selectedCellActor->GetProperty()->EdgeVisibilityOn();
    m_selectedCellActor->GetProperty()->SetEdgeColor(1, 0, 0);
    m_selectedCellActor->GetProperty()->SetLineWidth(3);

    GetDefaultRenderer()->AddActor(m_selectedCellActor);

    double point0[3];
    double point1[3];
    double point2[3];
    vtkFloatArray * selectedPoints = vtkFloatArray::SafeDownCast(selected->GetPoints()->GetData());
    selectedPoints->GetTuple(0, point0);
    selectedPoints->GetTuple(1, point1);
    selectedPoints->GetTuple(2, point2);


    // look at center of the object
    vtkPolyData * polyData = vtkPolyData::SafeDownCast(dataObject);
    double bounds[6];
    polyData->GetBounds(bounds);
    double boundsCenter[3];
    vtkMath::Add(&bounds[0], &bounds[3], boundsCenter);
    vtkMath::MultiplyScalar(boundsCenter, 0.5);
    GetDefaultRenderer()->GetActiveCamera()->SetFocalPoint(boundsCenter);

    // 
    double triCenter[3];
    vtkTriangle::TriangleCenter(point0, point1, point2, triCenter);
    double normal[3];
    vtkTriangle::ComputeNormal(point0, point1, point2, normal);
    float scale = 2 * std::max(bounds[1] - bounds[0], std::max(bounds[3] - bounds[2], bounds[6] - bounds[5]));

    vtkMath::MultiplyScalar(normal, scale);
    double eye[3];
    vtkMath::Subtract(triCenter, normal, eye);

    GetDefaultRenderer()->GetActiveCamera()->SetPosition(eye);

    GetDefaultRenderer()->GetRenderWindow()->Render();

    emit selectionChanged(cellId);
}

void PickingInteractionStyle::sendPointInfo() const
{
    double* pos = m_pointPicker->GetPickPosition();

    QString content;
    QTextStream stream;
    stream.setString(&content);

    stream.setRealNumberNotation(QTextStream::RealNumberNotation::ScientificNotation);
    stream.setRealNumberPrecision(17);

    std::string inputname;

    vtkAbstractMapper3D * mapper = m_pointPicker->GetMapper();

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
        << "id: " << m_pointPicker->GetPointId();

    QStringList info;
    QString line;
    while ((line = stream.readLine()) != QString())
        info.push_back(line);

    emit pointInfoSent(info);
}

void PickingInteractionStyle::changeSelection(const QItemSelection & selected, const QItemSelection & deselected)
{
    if (selected.indexes().isEmpty())
        return;

    highlightCell(selected.indexes().first().row(), m_mainDataObject);
}

void PickingInteractionStyle::setMainDataObject(vtkDataObject * dataObject)
{
    m_mainDataObject = dataObject;
}

